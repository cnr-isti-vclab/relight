"""
Multi-Lighting Inverse Rendering (SynthRTI) Renderer
Complete version with all functions included.
"""

import os
os.environ['OPENCV_IO_ENABLE_OPENEXR'] = '1'  # Keep for compatibility

import cv2
import glob
import lpips
import numpy as np
import torch
import torch.nn.functional as F
from scipy.ndimage import gaussian_filter
from skimage.restoration import denoise_bilateral
from skimage.metrics import structural_similarity as ssim
from typing import List, Tuple

torch.backends.cudnn.benchmark = True
DEVICE = torch.device("cuda" if torch.cuda.is_available() else "cpu")

# SynthRTI Dataset paths
SYNTHRTI_PATH = "/home/shakir/Project/ToyCode/SynthRTI"
RESULT_DIR = "synthrti_results"

# Dataset structure
OBJECTS = ["Object1", "Object2", "Object3"]
LIGHTING_TYPES = ["Single", "Multi"]  # Note: Capitalized in SynthRTI
MATERIALS = ["material1", "material2", "material3", "material4", "material5", "material6", "material7", "material8"]
# For Multi lighting, there's also material9
MATERIALS_MULTI = ["material1", "material2", "material3", "material4", "material5", "material6", "material7", "material8", "material9"]

# RealRTI Dataset paths
REALRTI_PATH = "/home/shakir/Project/ToyCode/RealRTI"
RESULT_DIR = "realrti_results"

# RealRTI structure: item1 ... item12
OBJECTS = sorted([d for d in os.listdir(REALRTI_PATH) 
                  if os.path.isdir(os.path.join(REALRTI_PATH, d))])


# Light intensity parameter - fixed initially
LIGHT_INTENSITY = 3.0
# Optimization parameters
NUM_ITERS = 800       # Total number of gradient descent iterations
LR = 5e-2            # Learning rate for Adam optimizer
LR_NORMALS = 1e-3    # VERY LOW LR for normals - only subtle refinements allowed
NORMAL_SMOOTH_WEIGHT = 2e-4  # Strong smoothness to prevent noise
NORMAL_INIT_WEIGHT = 3e-5  # Strong regularization to stay close to blended normals
LIGHT_CHUNK = 128    # Number of light directions to process in one chunk (for memory management)

# Initialize LPIPS model
lpips_model = lpips.LPIPS(net='alex').to(DEVICE)
lpips_model.eval()

# ============================================================================
# UTILITY FUNCTIONS
# ============================================================================

def apply_bilateral(img, sigma_s=5.0, sigma_c=0.5):
    """Apply bilateral filter to image."""
    from skimage.restoration import denoise_bilateral
    
    # Ensure image is in correct range [0,1]
    img_normalized = np.clip(img, 0, 1)
    
    # Apply bilateral filter
    filtered = denoise_bilateral(
        img_normalized, 
        sigma_color=sigma_c, 
        sigma_spatial=sigma_s,
        channel_axis=None  # Since it's a single channel image
    )
    
    return filtered

# Metric computations
def compute_psnr(img1, img2):
    mse = np.mean((img1 - img2) ** 2)
    if mse == 0:
        return float('inf')
    return 20 * np.log10(255.0 / np.sqrt(mse))

def compute_ssim(img1, img2):
    gray1 = cv2.cvtColor(img1, cv2.COLOR_RGB2GRAY)
    gray2 = cv2.cvtColor(img2, cv2.COLOR_RGB2GRAY)
    return ssim(gray1, gray2, data_range=gray2.max() - gray2.min())

def compute_lpips(img1, img2):
    img1_t = lpips.im2tensor(img1).to(DEVICE)
    img2_t = lpips.im2tensor(img2).to(DEVICE)
    with torch.no_grad():
        return lpips_model(img1_t, img2_t).item()

def load_image_stack(files: List[str]) -> np.ndarray:
    """
    Load images from SynthRTI dataset (JPG format).
    JPG images are in sRGB, need to convert to linear.
    """
    imgs = []
    for f in sorted(files):
        # Load JPG image
        img = cv2.imread(f, cv2.IMREAD_COLOR)
        if img is None:
            raise ValueError(f"Failed to load {f}")
        
        # Convert BGR to RGB
        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        
        # Convert to float32 and normalize to [0, 1]
        img = img.astype(np.float32) / 255.0
        
        # Convert from sRGB to linear RGB (inverse gamma correction)
        img_linear = srgb_to_linear(img)
        
        imgs.append(img_linear)
    
    return np.stack(imgs, axis=0).astype(np.float32)

def srgb_to_linear(srgb):
    """Convert sRGB to linear RGB."""
    linear = np.where(srgb <= 0.04045,
                      srgb / 12.92,
                      ((srgb + 0.055) / 1.055) ** 2.4)
    return np.clip(linear, 0, 1)

def linear_to_srgb(linear):
    """Convert linear RGB to sRGB."""
    srgb = np.where(linear <= 0.0031308,
                    12.92 * linear,
                    1.055 * np.power(np.clip(linear, 0.0031308, None), 1/2.4) - 0.055)
    return np.clip(srgb, 0, 1)

def load_light_directions(lp_path: str) -> np.ndarray:
    """
    Load light directions from a .lp file.
    SynthRTI format: each line has format: "x y z" or "index x y z"
    """
    dirs = []
    with open(lp_path) as f:
        lines = f.readlines()
        
        # Check first line to determine format
        first_line = lines[0].strip().split()
        
        if len(first_line) == 1 and first_line[0].isdigit():
            # Format: first line is number of lights
            n = int(first_line[0])
            start_idx = 1
        else:
            # Format: no count line, just directions
            n = len(lines)
            start_idx = 0
        
        for i in range(start_idx, start_idx + n):
            if i >= len(lines):
                break
            parts = lines[i].strip().split()
            if len(parts) >= 3:
                # Take last 3 parts (in case there's an index at the beginning)
                dirs.append(parts[-3:])
    
    L = np.asarray(dirs, np.float32)
    L /= np.linalg.norm(L, axis=1, keepdims=True) + 1e-8
    return L

def load_normals(normals_path: str) -> np.ndarray:
    """
    Load normals from normals.png (if available).
    Normals are stored as RGB image: (R,G,B) = (nx, ny, nz) mapped from [-1,1] to [0,255]
    """
    if not os.path.exists(normals_path):
        return None
    
    img = cv2.imread(normals_path, cv2.IMREAD_COLOR)
    if img is None:
        return None
    
    # Convert BGR to RGB
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    
    # Convert from [0,255] to [-1,1]
    normals = (img.astype(np.float32) / 255.0) * 2.0 - 1.0
    
    # Normalize to unit length
    norm = np.linalg.norm(normals, axis=2, keepdims=True)
    normals = normals / (norm + 1e-8)
    
    return normals

def create_default_light_directions(n_lights: int) -> np.ndarray:
    """Create default lighting directions if none provided."""
    # This is a placeholder - you should use actual light directions from the dataset
    np.random.seed(42)
    directions = np.random.randn(n_lights, 3)
    directions[:, 2] = np.abs(directions[:, 2])  # Ensure positive z (pointing down)
    directions = directions / np.linalg.norm(directions, axis=1, keepdims=True)
    return directions.astype(np.float32)

# ============================================================================
# PHOTOMETRIC STEREO FUNCTIONS
# ============================================================================

def lambertian_init(train_imgs: np.ndarray, lights: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
    """Lambertian initialization for normals and albedo."""
    K, H, W, _ = train_imgs.shape
    albedo = np.zeros((H * W, 3), np.float32)
    normals = np.zeros((H * W, 3, 3), np.float32)
    for c in range(3):
        channel = train_imgs[..., c].reshape(K, -1)
        nvec = np.linalg.lstsq(lights, channel, rcond=None)[0].T
        mag = np.linalg.norm(nvec, axis=1, keepdims=True) + 1e-6
        normals[:, :, c] = (nvec / mag)
        albedo[:, c] = mag[:, 0]
    avg_normals = np.mean(normals, axis=2)
    avg_normals /= np.linalg.norm(avg_normals, axis=1, keepdims=True) + 1e-6
    return avg_normals.reshape(H, W, 3), albedo.reshape(H, W, 3)

def robust_photometric_stereo_irls(images: np.ndarray, 
                                    light_dirs: np.ndarray,
                                    num_iterations: int = 5) -> np.ndarray:
    """Estimate normals using robust IRLS photometric stereo.
    Works for ALL materials (matte, specular, metallic)."""
    K, H, W, C = images.shape
    N_pixels = H * W
    
    print(f"    Robust IRLS Photometric Stereo ({num_iterations} iterations)")
    
    # Use all color channels (metals have colored specular)
    I_flat = images.reshape(K, N_pixels, C)  # (K, P, 3)
    I_lum = 0.2126 * I_flat[:, :, 0] + 0.7152 * I_flat[:, :, 1] + 0.0722 * I_flat[:, :, 2]
    
    L = light_dirs
    
    # Adaptive thresholds based on image statistics
    global_95 = np.percentile(I_lum, 95)
    bright_threshold = global_95 * 1.5
    dark_threshold = np.percentile(I_lum, 2) * 1.0
    specular_threshold = global_95 * 0.8
    
    # Initial robust estimate (using median of observations)
    print("    Computing robust initial estimate...")
    m = np.zeros((N_pixels, 3), dtype=np.float32)
    for p in range(N_pixels):
        # Use only non-bright observations for initial guess
        I_p = I_lum[:, p]
        valid_idx = I_p < bright_threshold
        if np.sum(valid_idx) >= 3:
            L_valid = L[valid_idx]
            I_valid = I_p[valid_idx]
            m[p] = np.linalg.lstsq(L_valid, I_valid, rcond=None)[0]
        else:
            # Fallback to all observations
            m[p] = np.linalg.lstsq(L, I_p, rcond=None)[0]
    
    # Initialize weights
    irls_w = np.ones((K, N_pixels), dtype=np.float32)
    
    # Validity mask (avoid too dark/bright)
    too_dark = I_lum < dark_threshold
    too_bright = I_lum > bright_threshold
    valid_mask = (~too_dark & ~too_bright).astype(np.float32)
    W_eff = valid_mask * irls_w
    
    for iteration in range(num_iterations):
        # Solve weighted least squares
        LtWL = np.einsum('ki,kj,kn->ijn', L, L, W_eff)
        LtWI = np.einsum('ki,kn->in', L, W_eff * I_lum)
        LtWL += 1e-5 * np.eye(3)[:, :, None]
        
        A = np.transpose(LtWL, (2, 0, 1))
        b = LtWI.T[:, :, np.newaxis]
        
        try:
            m_new = np.linalg.solve(A, b).squeeze(-1)
        except np.linalg.LinAlgError:
            m_new = m.copy()
        
        m_norm = np.linalg.norm(m_new, axis=1, keepdims=True) + 1e-8
        normals_flat = m_new / m_norm
        
        # Ensure normals point towards camera
        flip = normals_flat[:, 2] < 0
        normals_flat[flip] *= -1
        
        invalid_pix = m_norm.squeeze() < 1e-6
        normals_flat[invalid_pix] = [0, 0, 1]
        
        if iteration < num_iterations - 1:
            # Compute residuals
            rho = m_norm.squeeze()
            NdotL = np.maximum(normals_flat @ L.T, 0).T
            I_pred = rho[None, :] * NdotL
            residual = I_lum - I_pred
            
            # Robust statistics
            abs_res = np.abs(residual)
            mad = np.median(abs_res, axis=0, keepdims=True) * 1.4826 + 1e-6
            
            # Tukey's biweight (robust to moderate outliers)
            c = 4.685
            u = residual / mad
            tukey_w = np.where(np.abs(u) < c, (1 - (u / c) ** 2) ** 2, 0.0)
            
            # ℓ₁-inspired weight for extreme outliers
            l1_w = 1.0 / (np.abs(residual) + 1e-4)
            l1_w = np.clip(l1_w, 0.01, 1.0)
            
            # Penalties for extreme observations
            specular_penalty = np.where(residual > specular_threshold, 0.3, 1.0)  
            bright_penalty = np.where(I_lum > bright_threshold, 0.1, 1.0)
            dark_penalty = np.where(I_lum < dark_threshold, 0.1, 1.0)
            
            # Combine weights
            irls_w = tukey_w * l1_w * specular_penalty * bright_penalty * dark_penalty
            irls_w = np.clip(irls_w, 0.01, 1.0)
            
            # Update effective weights
            W_eff = valid_mask * irls_w
            
            # Ensure enough observations per pixel
            obs_count = np.sum(W_eff > 0.1, axis=0)
            too_few = obs_count < 4
            W_eff[:, too_few] = valid_mask[:, too_few] * 0.3
            
            m = m_new
    
    normals = normals_flat.reshape((H, W, 3))
    norm_len = np.linalg.norm(normals, axis=2)
    invalid = norm_len < 0.5
    normals[invalid] = [0, 0, 1]
    
    return normals.astype(np.float32)

def simple_lambertian_photometric_stereo(images: np.ndarray, light_dirs: np.ndarray) -> np.ndarray:
    """Simple Lambertian photometric stereo without IRLS weighting."""
    K, H, W, _ = images.shape
    N_pixels = H * W
    
    print(f"    Simple Lambertian Photometric Stereo")
    
    I_lum = 0.2126 * images[:, :, :, 0] + 0.7152 * images[:, :, :, 1] + 0.0722 * images[:, :, :, 2]
    I_flat = I_lum.reshape(K, N_pixels)
    
    L = light_dirs
    LtL = L.T @ L
    LtL_inv = np.linalg.inv(LtL + 1e-5 * np.eye(3))
    
    m = LtL_inv @ L.T @ I_flat
    m = m.T
    
    m_norm = np.linalg.norm(m, axis=1, keepdims=True) + 1e-8
    normals_flat = m / m_norm
    
    flip = normals_flat[:, 2] < 0
    normals_flat[flip] *= -1
    
    invalid_pix = m_norm.squeeze() < 1e-6
    normals_flat[invalid_pix] = [0, 0, 1]
    
    normals = normals_flat.reshape((H, W, 3))
    norm_len = np.linalg.norm(normals, axis=2)
    invalid = norm_len < 0.5
    normals[invalid] = [0, 0, 1]
    
    return normals.astype(np.float32)

# ============================================================================
# MATERIAL ANALYSIS FUNCTIONS
# ============================================================================

def detect_metallic_regions(images: np.ndarray, threshold_ratio: float = 8.0,
                            min_region_size: int = 500,
                            is_single_material: bool = True,
                            iit_images: np.ndarray = None) -> np.ndarray:
    """Detect metallic regions based on specular behavior with noise removal."""
    from scipy import ndimage
    
    K, H, W, C = images.shape
    total_pixels = H * W
    
    # Use IIT images if provided
    detect_imgs = iit_images if iit_images is not None else images
    
    print(f"    Detecting metallic regions (single_material={is_single_material}, using_iit={iit_images is not None})...")
    
    # Use detect_imgs for all intensity calculations
    I_max = np.max(detect_imgs, axis=3)
    max_intensity = np.max(I_max, axis=0)
    median_intensity = np.median(I_max, axis=0)
    mean_intensity = np.mean(I_max, axis=0)
    
    with np.errstate(divide='ignore', invalid='ignore'):
        ratio = max_intensity / (median_intensity + 1e-6)
    
    metallic_raw = np.clip((ratio - threshold_ratio) / threshold_ratio, 0, 1)
    brightness_penalty = np.clip(1.0 - mean_intensity / 0.4, 0, 1)
    metallic_raw = metallic_raw * brightness_penalty
    
    # Use bilateral filter to preserve edges
    metallic_mask = denoise_bilateral(metallic_raw, sigma_color=0.1, sigma_spatial=2.0)

    metal_pixels_raw = np.sum(metallic_mask > 0.5)
    print(f"      Max/median ratio range: [{ratio.min():.2f}, {ratio.max():.2f}]")
    print(f"      Raw metallic pixels: {metal_pixels_raw}/{total_pixels} ({100*metal_pixels_raw/total_pixels:.1f}%)")
    
    # Morphological cleanup
    binary_mask = (metallic_mask > 0.5).astype(np.uint8)
    
    # Remove small metallic regions
    labeled_metal, num_metal = ndimage.label(binary_mask)
    for i in range(1, num_metal + 1):
        region_size = np.sum(labeled_metal == i)
        if region_size < min_region_size:
            binary_mask[labeled_metal == i] = 0
    
    # Remove small dielectric regions (holes in metal)
    binary_dielectric = 1 - binary_mask
    labeled_dielectric, num_dielectric = ndimage.label(binary_dielectric)
    for i in range(1, num_dielectric + 1):
        region_size = np.sum(labeled_dielectric == i)
        if region_size < min_region_size:
            binary_mask[labeled_dielectric == i] = 1
    
    struct = ndimage.generate_binary_structure(2, 1)
    binary_mask = ndimage.binary_closing(binary_mask, struct, iterations=3)
    binary_mask = ndimage.binary_opening(binary_mask, struct, iterations=2)
    
    metallic_mask = binary_mask.astype(np.float32)
    metal_pixels = np.sum(metallic_mask > 0.5)
    metal_percentage = 100 * metal_pixels / total_pixels
    
    print(f"      After cleanup: {metal_pixels}/{total_pixels} ({metal_percentage:.1f}%)")
    
    # Handle single material case
    if is_single_material:
        if metal_percentage < 40.0:
            print(f"      Single material: {metal_percentage:.1f}% < 40% -> FULLY MATTE")
            metallic_mask = np.zeros((H, W), dtype=np.float32)
        elif metal_percentage > 80.0:
            print(f"      Single material: {metal_percentage:.1f}% > 80% -> FULLY METALLIC")
            metallic_mask = np.ones((H, W), dtype=np.float32)
        else:
            # Ambiguous case - use image statistics to decide
            overall_max_median_ratio = np.median(ratio)
            if overall_max_median_ratio < threshold_ratio * 0.8:
                print(f"      Single material: ambiguous but low overall ratio ({overall_max_median_ratio:.2f}) -> FULLY MATTE")
                metallic_mask = np.zeros((H, W), dtype=np.float32)
            else:
                print(f"      Single material: ambiguous, high ratio ({overall_max_median_ratio:.2f}) -> FULLY METALLIC")
                metallic_mask = np.ones((H, W), dtype=np.float32)
    
    final_metal_pct = 100 * np.mean(metallic_mask)
    print(f"      Final: {final_metal_pct:.1f}% metallic")
    
    return metallic_mask

def blend_normals(normals_dielectric: np.ndarray, 
                  normals_metal: np.ndarray, 
                  metallic_mask: np.ndarray) -> np.ndarray:
    """Blend two normal maps based on metallic mask."""
    H, W, _ = normals_dielectric.shape
    
    mask = metallic_mask[:, :, np.newaxis]
    blended = (1.0 - mask) * normals_dielectric + mask * normals_metal
    
    norm_len = np.linalg.norm(blended, axis=2, keepdims=True) + 1e-8
    blended = blended / norm_len
    
    invalid = norm_len.squeeze() < 0.5
    blended[invalid] = [0, 0, 1]
    
    return blended.astype(np.float32)

def analyze_specularity(images: np.ndarray, lights: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
    """Analyze specularity for F0 and roughness initialization."""
    K, H, W, C = images.shape
    
    print("    Analyzing specularity for F0/roughness initialization...")
    # Compute luminance
    I_lum = 0.2126 * images[:, :, :, 0] + 0.7152 * images[:, :, :, 1] + 0.0722 * images[:, :, :, 2]
    
    # Statistics across lighting conditions
    I_max = np.max(I_lum, axis=0)
    I_min = np.min(I_lum, axis=0)
    I_mean = np.mean(I_lum, axis=0)
    I_std = np.std(I_lum, axis=0)
    
    with np.errstate(divide='ignore', invalid='ignore'):
        highlight_ratio = I_max / (I_mean + 1e-6)
        cv = I_std / (I_mean + 1e-6)
    
    # Detect specular regions
    specular_score = np.clip((highlight_ratio - 1.5) / 3.0, 0, 1) * np.clip(cv / 0.5, 0, 1)
    
    # Smooth the mask
    specular_mask = apply_bilateral(specular_score, sigma_s=3.0, sigma_c=0.15)
    specular_mask = np.clip(specular_mask, 0, 1)
    
    # Estimate roughness from highlight sharpness
    intensity_range = I_max - I_min
    with np.errstate(divide='ignore', invalid='ignore'):
        sharpness = intensity_range / (I_mean + 1e-6)
    
    # Map sharpness to roughness
    roughness_init = np.clip(1.0 - (sharpness - 0.5) / 4.0, 0.1, 0.95)
    roughness_init = apply_bilateral(roughness_init, sigma_s=2.0, sigma_c=0.1)
    
    spec_pct = 100 * np.mean(specular_mask > 0.3)
    rough_mean = np.mean(roughness_init)
    print(f"      Specular regions: {spec_pct:.1f}%")
    print(f"      Mean roughness estimate: {rough_mean:.3f}")
    print(f"      Highlight ratio range: [{highlight_ratio.min():.2f}, {highlight_ratio.max():.2f}]")
    
    return specular_mask.astype(np.float32), roughness_init.astype(np.float32)

def estimate_albedo_lambertian(images: np.ndarray, normals: np.ndarray,
                                lights: np.ndarray) -> np.ndarray:
    """Estimate albedo using Lambertian model."""
    K, H, W, C = images.shape
    P = H * W
    print("    Estimating Lambertian albedo...")
    
    # Compute N·L for all pixels and lights
    N = normals.reshape(P, 3)
    L = lights
    NdotL = np.maximum(N @ L.T, 0)
    
    # Solve for albedo using least squares
    images_flat = images.reshape(K, P, C)
    
    albedo = np.zeros((P, C), dtype=np.float32)
    
    for c in range(C):
        I_channel = images_flat[:, :, c].T
        
        numerator = np.sum(I_channel * NdotL, axis=1)
        denominator = np.sum(NdotL * NdotL, axis=1) + 1e-6
        
        albedo[:, c] = numerator / denominator
    
    albedo = np.clip(albedo, 0.001, 1.0)
    albedo = albedo.reshape(H, W, C)
    
    print(f"      Albedo range: [{albedo.min():.4f}, {albedo.max():.4f}]")
    
    return albedo.astype(np.float32)

def value_to_logit(value: np.ndarray, low: float, high: float) -> np.ndarray:
    """Convert value to logit space for optimization."""
    norm = (value - low) / (high - low)
    norm = np.clip(norm, 1e-4, 1 - 1e-4)
    return np.log(norm / (1 - norm)).astype(np.float32)

# ============================================================================
# RENDERING FUNCTIONS
# ============================================================================

def render_ggx(normals, albedo, roughness, f0, lights, light_intensity, metallic_mask=None):
    """GGX microfacet BRDF renderer."""
    device = normals.device
    N = F.normalize(normals, dim=-1).unsqueeze(0)          # 1 x P x 3
    A = torch.clamp(albedo, 0.0, 1.0).unsqueeze(0)         # 1 x P x 3
    rough = torch.clamp(roughness, 0.02, 1.0).unsqueeze(0) # 1 x P x 1
    alpha = rough * rough
    F0 = torch.clamp(f0, 0.0, 1.0).unsqueeze(0)            # 1 x P x 3

    L = F.normalize(lights, dim=-1).unsqueeze(1)           # K x 1 x 3
    V = torch.tensor([0.0, 0.0, 1.0], device=device).view(1, 1, 3)

    H = F.normalize(L + V, dim=-1)
    NdotL = torch.clamp((N * L).sum(-1), min=0.0)
    NdotV = torch.clamp((N * V).sum(-1), min=1e-4)
    NdotH = torch.clamp((N * H).sum(-1), min=1e-4)
    VdotH = torch.clamp((V * H).sum(-1), min=1e-4)

    # GGX Distribution
    alpha2 = (alpha * alpha).squeeze(-1).expand_as(NdotH)
    denom = (NdotH * NdotH) * (alpha2 - 1.0) + 1.0
    D = alpha2 / (np.pi * denom * denom + 1e-7)

    # Smith G term
    alpha_expand = alpha.squeeze(-1).expand_as(NdotL)
    NdotV_expand = NdotV.expand_as(NdotL)
    gv = NdotV_expand + torch.sqrt(alpha_expand * alpha_expand +
                                   (1 - alpha_expand * alpha_expand) * NdotV_expand * NdotV_expand + 1e-7)
    gl = NdotL + torch.sqrt(alpha_expand * alpha_expand +
                            (1 - alpha_expand * alpha_expand) * NdotL * NdotL + 1e-7)
    G = 1.0 / (gv * gl + 1e-7)

    # Fresnel (Schlick)
    F_term = F0.unsqueeze(0) + (1.0 - F0.unsqueeze(0)) * torch.pow(torch.clamp(1.0 - VdotH.unsqueeze(-1), 0.0, 1.0), 5.0)

    # Specular term
    spec = (D * G).unsqueeze(-1) * F_term / (4.0 * NdotV_expand.unsqueeze(-1) * NdotL.unsqueeze(-1) + 1e-6)

    # Diffuse and ambient
    diff = (1.0 - F_term) * A / np.pi
    ambient = 0.01 * A
    
    # Safe intensity clamping
    if torch.is_tensor(light_intensity):
        safe_intensity = torch.clamp(light_intensity, min=0.1)
    else:
        safe_intensity = max(float(light_intensity), 0.1)
    
    # Combine components
    shade = safe_intensity * (
        diff * NdotL.unsqueeze(-1) +
        spec * NdotL.unsqueeze(-1) +
        ambient * NdotL.unsqueeze(-1)
    )
    return torch.clamp(shade, min=0.0)

def fit_parameters(train_imgs, lights, init_normals, init_albedo, specular_mask, roughness_init, metallic_mask=None):
    """Optimization loop to fit the parameters."""
    if metallic_mask is None:
        metallic_mask = np.zeros((init_normals.shape[0], init_normals.shape[1]), dtype=np.float32)
    
    K, H, W, _ = train_imgs.shape
    P = H * W
    
    # Specular mask for diffuse loss masking
    spec_mask_flat = torch.from_numpy(specular_mask.reshape(P, 1)).to(DEVICE)
    
    # Memory optimization: process lights in chunks
    CHUNK_SIZE = min(16, K)

    # Keep training data on CPU, move chunks to GPU during forward pass
    train_np = train_imgs.reshape(K, P, 3)
    lights_t = torch.from_numpy(lights).to(DEVICE)

    # Initialize parameters
    normals_param = torch.nn.Parameter(torch.from_numpy(init_normals.reshape(P, 3)).to(DEVICE))
    
    # ALBEDO IS FIXED - NOT OPTIMIZED!
    albedo_fixed = torch.from_numpy(init_albedo.reshape(P, 3)).to(DEVICE)
    
    # Roughness initialization
    rough_init = roughness_init.reshape(P, 1)
    rough_logit = torch.nn.Parameter(torch.from_numpy(value_to_logit(rough_init, 0.02, 1.0)).to(DEVICE))
    
    # F0 initialization
    spec_flat = specular_mask.reshape(P, 1)
    f0_init = 0.04 + 0.4 * spec_flat
    f0_init = np.repeat(f0_init, 3, axis=1)
    f0_logit = torch.nn.Parameter(torch.from_numpy(value_to_logit(f0_init, 0.02, 0.98)).to(DEVICE))
    
    # Store masks as tensors
    metal_flat = metallic_mask.reshape(P)
    roughness_target_t = torch.from_numpy(rough_init).to(DEVICE)
    init_normals_t = torch.from_numpy(init_normals.reshape(P, 3)).to(DEVICE)
    
    # Learnable light intensity
    intensity_param = torch.nn.Parameter(torch.tensor([np.log(LIGHT_INTENSITY)], dtype=torch.float32, device=DEVICE))

    # Optimizer with separate learning rates
    optimizer = torch.optim.Adam([
        {'params': [normals_param], 'lr': LR_NORMALS},
        {'params': [rough_logit, f0_logit], 'lr': LR},
        {'params': [intensity_param], 'lr': LR * 5}
    ])
    
    def compute_normal_smoothness(normals_flat, H, W):
        """Compute spatial smoothness loss for normals."""
        N = normals_flat.reshape(H, W, 3)
        diff_h = N[:, 1:, :] - N[:, :-1, :]
        diff_v = N[1:, :, :] - N[:-1, :, :]
        smooth_loss = torch.mean(diff_h ** 2) + torch.mean(diff_v ** 2)
        return smooth_loss

    # Simplified diffuse component - ignore metallic mask
    def render_diffuse_only(normals, albedo, roughness, f0, lights, intensity):
        N = F.normalize(normals, dim=-1).unsqueeze(0)
        A = torch.clamp(albedo, 0.0, 1.0).unsqueeze(0)
        F0 = torch.clamp(f0, 0.0, 1.0).unsqueeze(0)

        L = F.normalize(lights, dim=-1).unsqueeze(1)
        V = torch.tensor([0.0, 0.0, 1.0], device=N.device).view(1, 1, 3)
        H_dir = F.normalize(L + V, dim=-1)

        NdotL = torch.clamp((N * L).sum(-1), min=0.0)
        VdotH = torch.clamp((V * H_dir).sum(-1), min=0.0)
        F_term = F0.unsqueeze(0) + (1 - F0.unsqueeze(0)) * (1 - VdotH.unsqueeze(-1))**5

        # Simple diffuse for all materials - no metal-specific handling
        diff = (1.0 - F_term) * A / np.pi
        
        return intensity * diff * NdotL.unsqueeze(-1)

    # Specular component - same for all materials
    def render_specular_only(normals, roughness, f0, lights, intensity):
        N = F.normalize(normals, dim=-1).unsqueeze(0)
        rough = torch.clamp(roughness, 0.02, 1.0).unsqueeze(0)
        alpha = rough * rough
        F0 = torch.clamp(f0, 0.0, 1.0).unsqueeze(0)
        
        L = F.normalize(lights, dim=-1).unsqueeze(1)
        V = torch.tensor([0.0, 0.0, 1.0], device=N.device).view(1, 1, 3)
        H_dir = F.normalize(L + V, dim=-1)
        
        NdotL = torch.clamp((N * L).sum(-1), min=0.0)
        NdotV = torch.clamp((N * V).sum(-1), min=1e-4)
        NdotH = torch.clamp((N * H_dir).sum(-1), min=1e-4)
        VdotH = torch.clamp((V * H_dir).sum(-1), min=1e-4)
        
        # GGX Distribution
        alpha2 = (alpha * alpha).squeeze(-1).expand_as(NdotH)
        denom = (NdotH * NdotH) * (alpha2 - 1.0) + 1.0
        D = alpha2 / (np.pi * denom * denom + 1e-7)
        
        # Smith G term
        alpha_expand = alpha.squeeze(-1).expand_as(NdotL)
        NdotV_expand = NdotV.expand_as(NdotL)
        gv = NdotV_expand + torch.sqrt(alpha_expand * alpha_expand +
                                       (1 - alpha_expand * alpha_expand) * NdotV_expand * NdotV_expand + 1e-7)
        gl = NdotL + torch.sqrt(alpha_expand * alpha_expand +
                                (1 - alpha_expand * alpha_expand) * NdotL * NdotL + 1e-7)
        G = 1.0 / (gv * gl + 1e-7)
        
        # Fresnel
        F_term = F0.unsqueeze(0) + (1.0 - F0.unsqueeze(0)) * torch.pow(torch.clamp(1.0 - VdotH.unsqueeze(-1), 0.0, 1.0), 5.0)
        
        spec = (D * G).unsqueeze(-1) * F_term / (4.0 * NdotV_expand.unsqueeze(-1) * NdotL.unsqueeze(-1) + 1e-6)
        return intensity * spec * NdotL.unsqueeze(-1)

    # Simplified ambient - no metal-specific
    def render_ambient_only(normals, albedo, lights, intensity):
        N = F.normalize(normals, dim=-1).unsqueeze(0)
        A = torch.clamp(albedo, 0.0, 1.0).unsqueeze(0)
        
        L = F.normalize(lights, dim=-1).unsqueeze(1)
        NdotL = torch.clamp((N * L).sum(-1), min=0.0)
        
        # Small ambient for all materials
        ambient = 0.01 * A
        
        return intensity * ambient * NdotL.unsqueeze(-1)

    # Training loop
    for step in range(NUM_ITERS):
        optimizer.zero_grad()
        
        # Get current parameters
        normals = F.normalize(normals_param, dim=-1)
        albedo = albedo_fixed
        
        # Simple roughness for ALL materials - no metal-specific adjustments
        roughness = 0.02 + 0.98 * torch.sigmoid(rough_logit)
        
        f0 = 0.02 + 0.96 * torch.sigmoid(f0_logit)
        intensity = torch.exp(intensity_param)[0] 

        total_loss_full = 0.0
        total_loss_diffuse = 0.0
        
        # Process in chunks
        for chunk_start in range(0, K, CHUNK_SIZE):
            chunk_end = min(chunk_start + CHUNK_SIZE, K)
            lights_chunk = lights_t[chunk_start:chunk_end]
            train_chunk = torch.from_numpy(train_np[chunk_start:chunk_end]).to(DEVICE)
            
            # Render components
            diffuse_chunk = render_diffuse_only(normals, albedo, roughness, f0, lights_chunk, intensity)
            specular_chunk = render_specular_only(normals, roughness, f0, lights_chunk, intensity)
            ambient_chunk = render_ambient_only(normals, albedo, lights_chunk, intensity)
            
            # Remove extra dimension
            diffuse_chunk = diffuse_chunk.squeeze(0)
            specular_chunk = specular_chunk.squeeze(0)
            ambient_chunk = ambient_chunk.squeeze(0)
            
            # Full render
            full_chunk = diffuse_chunk + specular_chunk + ambient_chunk
            
            # Simple loss weighting - no metal-specific
            non_spec_weight = (1.0 - spec_mask_flat)
            
            # Compute losses
            total_loss_full += F.l1_loss(
                full_chunk,
                train_chunk,
                reduction='sum')

            total_loss_diffuse += F.l1_loss(
                diffuse_chunk * non_spec_weight,
                train_chunk * non_spec_weight,
                reduction='sum')
        
        # Normalize losses
        loss_full = total_loss_full / (K * P * 3)
        loss_diffuse = total_loss_diffuse / (K * P * 3)
        loss_recon = loss_full + 0.2 * loss_diffuse
        
        # Simple regularization - no metal-specific
        rough_reg = 1e-3 * torch.mean((roughness - roughness_target_t) ** 2)
        f0_reg = 5e-4 * torch.mean((f0 - 0.04) ** 2)
        intensity_reg = 1e-6 * (intensity - LIGHT_INTENSITY) ** 2
        normal_smooth_reg = NORMAL_SMOOTH_WEIGHT * compute_normal_smoothness(normals, H, W)
        cosine_sim = torch.sum(normals * init_normals_t, dim=-1)
        normal_init_reg = NORMAL_INIT_WEIGHT * torch.mean((1.0 - cosine_sim) ** 2)
        
        # Total loss
        loss = (loss_recon + rough_reg + f0_reg + intensity_reg + 
                normal_smooth_reg + normal_init_reg)
        
        loss.backward()
        optimizer.step()

        # Progress reporting
        if (step + 1) % 100 == 0:
            with torch.no_grad():
                mean_f0 = f0.mean().item()
                mean_rough = roughness.mean().item()
                learned_intensity = intensity.item()
            print(f"  iter {step + 1}/{NUM_ITERS}  loss={loss.item():.6f}")
            print(f"      F0={mean_f0:.4f}, rough={mean_rough:.3f}, intensity={learned_intensity:.4f}")

    # Extract final parameters
    with torch.no_grad():
        optimized_normals = F.normalize(normals_param, dim=-1).cpu().numpy().reshape(H, W, 3)
        albedo_out = init_albedo.reshape(H, W, 3)
        
        # Simple roughness for all materials
        roughness_final = (0.02 + 0.98 * torch.sigmoid(rough_logit)).cpu().numpy().reshape(H, W, 1)
        
        f0_temp = (0.02 + 0.96 * torch.sigmoid(f0_logit)).cpu().numpy()
        f0 = f0_temp.reshape(H, W, 3)
        learned_intensity = torch.exp(intensity_param).detach().cpu().numpy()[0]
    
    torch.cuda.empty_cache()

    return {
        "normals_map": optimized_normals,
        "albedo_map": albedo_out,
        "roughness_map": roughness_final,
        "f0_map": f0,
        "intensity": learned_intensity,
        "metallic_mask": metallic_mask
    }


def render_with_params(params, lights):
    """Render images using learned parameters."""
    H, W, _ = params["albedo_map"].shape
    P = H * W
    K = len(lights)
    
    normals = torch.from_numpy(params["normals_map"].reshape(P, 3)).to(DEVICE)
    albedo = torch.from_numpy(params["albedo_map"].reshape(P, 3)).to(DEVICE)
    roughness = torch.from_numpy(params["roughness_map"].reshape(P, 1)).to(DEVICE)
    f0 = torch.from_numpy(params["f0_map"].reshape(P, 3)).to(DEVICE)
    
    # Get metallic mask if available
    if "metallic_mask" in params:
        metallic_mask = torch.from_numpy(params["metallic_mask"].reshape(P, 1)).to(DEVICE)
    else:
        metallic_mask = torch.zeros((P, 1), device=DEVICE)
    
    lights_t = torch.from_numpy(lights).to(DEVICE)
    intensity = params.get("intensity", LIGHT_INTENSITY)
    
    with torch.no_grad():
        render = render_ggx(normals, albedo, roughness, f0, lights_t, intensity, metallic_mask)
    
    renders = render.cpu().numpy().reshape(K, H, W, 3)
    return renders


def process_material(obj_name, lighting_type, material_name, result_base_path, global_log):
    """
    Process one material from SynthRTI dataset.
    """
    print(f"\n{'='*70}")
    print(f"Processing: {obj_name} - {lighting_type} - {material_name}")
    print(f"{'='*70}")

    material_result_dir = os.path.join(result_base_path, obj_name, lighting_type, material_name)
    os.makedirs(material_result_dir, exist_ok=True)

    # Paths for SynthRTI
    obj_path = os.path.join(SYNTHRTI_PATH, lighting_type, obj_name, material_name)
    
    # Training images (Dome directory - 49 images)
    train_path = os.path.join(obj_path, "Dome")
    train_files = sorted(glob.glob(os.path.join(train_path, "*.jpg")))
    
    if not train_files:
        print(f"  No training images found at {train_path}")
        return
    
    # Expect 49 training images (Dome lighting)
    train_imgs = load_image_stack(train_files)
    K, H, W, C = train_imgs.shape
    print(f"  Loaded {K} training images from Dome, shape: {H}x{W}")
    print(f"  Value range (linear): [{train_imgs.min():.4f}, {train_imgs.max():.4f}]")
    
    # Normalize HDR images (though JPGs are already in [0,1] range)
    img_scale = np.percentile(train_imgs, 99)
    if img_scale > 1.0:
        train_imgs_norm = train_imgs / img_scale
    else:
        train_imgs_norm = train_imgs
        img_scale = 1.0
    train_imgs_norm = np.clip(train_imgs_norm, 0, 1)
    
    # Load light directions from Dome directory
    lp_file = os.path.join(train_path, "dirs.lp")
    if os.path.exists(lp_file):
        lights_train = load_light_directions(lp_file)
        print(f"  Loaded {len(lights_train)} light directions from {lp_file}")
    else:
        print(f"  Warning: No light directions file found at {lp_file}")
        # Create default lighting directions if needed
        lights_train = create_default_light_directions(49)
    
    # Try to load ground truth normals if available
    normals_gt_path = os.path.join(train_path, "normals.png")
    normals_gt = load_normals(normals_gt_path)
    if normals_gt is not None:
        print(f"  Loaded ground truth normals from {normals_gt_path}")
        # Save ground truth normals for comparison
        normals_gt_vis = ((normals_gt + 1.0) / 2.0 * 255).clip(0, 255).astype(np.uint8)
        cv2.imwrite(os.path.join(material_result_dir, "normal_map_gt.png"),
                    cv2.cvtColor(normals_gt_vis, cv2.COLOR_RGB2BGR))
    
    # 1. Compute IRLS normals (robust, for dielectrics/matte)
    print("  Computing IRLS normals (robust for matte materials)...")
    irls_normals = robust_photometric_stereo_irls(train_imgs_norm, lights_train, num_iterations=5)

    # 2. Compute simple Lambertian normals (for metals)
    print("  Computing Lambertian normals (for metallic materials)...")
    lambertian_normals = simple_lambertian_photometric_stereo(train_imgs_norm, lights_train)

    # 3. Detect metallic regions
    is_single_material = (lighting_type == "Single")
    print(f"  Detecting metallic regions (is_single_material={is_single_material})...")
    metallic_mask = detect_metallic_regions(train_imgs_norm, threshold_ratio=8.0, 
                                            min_region_size=500,
                                            is_single_material=is_single_material)

    # 4. Blend normals
    print("  Blending normals (IRLS for dielectrics, Lambertian for metals)...")
    init_normals = blend_normals(irls_normals, lambertian_normals, metallic_mask)
    
    # Debug: Report which normals are being used
    metal_pct = np.mean(metallic_mask) * 100
    if metal_pct > 50:
        print(f"    -> Material detected as METALLIC ({metal_pct:.1f}%) -> using LAMBERTIAN normals as init")
    else:
        print(f"    -> Material detected as DIELECTRIC ({100-metal_pct:.1f}%) -> using IRLS normals as init")
    
    # Save metallic mask
    cv2.imwrite(os.path.join(material_result_dir, "metallic_mask.png"),
                (metallic_mask * 255).clip(0, 255).astype(np.uint8))
    
    # Compute median color (90th percentile) for albedo initialization
    print("  Computing median color for albedo initialization...")
    median_color = np.percentile(train_imgs_norm, 90, axis=0)
    median_color = median_color / (np.percentile(median_color, 99) + 1e-6)
    init_albedo = np.clip(median_color, 0.05, 1.0).astype(np.float32)

    print(f"    Median color range: [{init_albedo.min():.4f}, {init_albedo.max():.4f}]")

    # Save median color for visualization
    median_color_srgb = linear_to_srgb(init_albedo)
    cv2.imwrite(os.path.join(material_result_dir, "albedo_median.png"),
                cv2.cvtColor((median_color_srgb * 255).clip(0, 255).astype(np.uint8), cv2.COLOR_RGB2BGR))
    
    # Analyze specularity for proper F0/roughness initialization
    print("  Analyzing material specularity...")
    specular_mask, roughness_init = analyze_specularity(train_imgs_norm, lights_train)
    
    # Save specular analysis
    cv2.imwrite(os.path.join(material_result_dir, "specular_mask.png"),
                (specular_mask * 255).clip(0, 255).astype(np.uint8))
    cv2.imwrite(os.path.join(material_result_dir, "roughness_init.png"),
                (roughness_init * 255).clip(0, 255).astype(np.uint8))

    # FIT PARAMETERS (with learnable intensity)
    params = fit_parameters(train_imgs_norm, lights_train, init_normals, init_albedo, 
                           specular_mask, roughness_init, metallic_mask=metallic_mask)
    params["img_scale"] = img_scale  # Store for relighting
    
    print(f"  Learned intensity: {params['intensity']:.4f}")

    # SAVE MATERIAL MAPS
    # Save IRLS normals
    irls_normals_vis = ((irls_normals + 1.0) / 2.0 * 255).clip(0, 255).astype(np.uint8)
    cv2.imwrite(os.path.join(material_result_dir, "normal_map_irls.png"),
                cv2.cvtColor(irls_normals_vis, cv2.COLOR_RGB2BGR))
    np.save(os.path.join(material_result_dir, "normals_irls.npy"), irls_normals)
    
    # Save Lambertian normals
    lambertian_normals_vis = ((lambertian_normals + 1.0) / 2.0 * 255).clip(0, 255).astype(np.uint8)
    cv2.imwrite(os.path.join(material_result_dir, "normal_map_lambertian.png"),
                cv2.cvtColor(lambertian_normals_vis, cv2.COLOR_RGB2BGR))
    np.save(os.path.join(material_result_dir, "normals_lambertian.npy"), lambertian_normals)
    
    # Save blended normals (init_normals before optimization)
    blended_normals_vis = ((init_normals + 1.0) / 2.0 * 255).clip(0, 255).astype(np.uint8)
    cv2.imwrite(os.path.join(material_result_dir, "normal_map_blended.png"),
                cv2.cvtColor(blended_normals_vis, cv2.COLOR_RGB2BGR))
    np.save(os.path.join(material_result_dir, "normals_blended.npy"), init_normals)
    
    # Save OPTIMIZED normals
    optimized_normals = params["normals_map"]
    optimized_normals_vis = ((optimized_normals + 1.0) / 2.0 * 255).clip(0, 255).astype(np.uint8)
    cv2.imwrite(os.path.join(material_result_dir, "normal_map_optimized.png"),
                cv2.cvtColor(optimized_normals_vis, cv2.COLOR_RGB2BGR))
    np.save(os.path.join(material_result_dir, "normals_optimized.npy"), optimized_normals)
    
    # Save albedo
    albedo_srgb = linear_to_srgb(params["albedo_map"])
    cv2.imwrite(os.path.join(material_result_dir, "albedo.png"),
                cv2.cvtColor((albedo_srgb * 255).clip(0, 255).astype(np.uint8), cv2.COLOR_RGB2BGR))
    np.save(os.path.join(material_result_dir, "albedo.npy"), params["albedo_map"])
    
    # Save roughness
    cv2.imwrite(os.path.join(material_result_dir, "roughness.png"),
                (params["roughness_map"].squeeze(-1) * 255).clip(0, 255).astype(np.uint8))
    np.save(os.path.join(material_result_dir, "roughness.npy"), params["roughness_map"])
    
    # Save F0
    np.save(os.path.join(material_result_dir, "f0.npy"), params["f0_map"])
    print(f"  Saved material maps")

    # EVALUATION ON TEST SET
    test_path = os.path.join(obj_path, "Test")
    test_files = sorted(glob.glob(os.path.join(test_path, "*.jpg")))
    
    if not test_files:
        print(f"  No test images found at {test_path}")
        return
    
    # Load test light directions
    test_lp_file = os.path.join(test_path, "dirs.lp")
    if os.path.exists(test_lp_file):
        lights_test = load_light_directions(test_lp_file)
    else:
        print(f"  Warning: No test light directions found, using training lights")
        lights_test = lights_train
    
    num_test = min(len(test_files), len(lights_test))
    test_imgs = load_image_stack(test_files[:num_test])
    print(f"    Evaluating {num_test} test images...")
    
    # Create test result directory
    test_result_dir = os.path.join(material_result_dir, "test_results")
    os.makedirs(test_result_dir, exist_ok=True)
    
    # Render with fitted parameters
    renders = render_with_params(params, lights_test[:num_test])
    
    psnr_vals, ssim_vals, lpips_vals = [], [], []
    
    for idx in range(num_test):
        gt_img = test_imgs[idx]
        relight = renders[idx] * img_scale  # Scale back
        
        # Tone mapping (Reinhard)
        gt_tm = gt_img / (1.0 + gt_img)
        relight_tm = relight / (1.0 + relight)
        
        # Convert to sRGB for display
        gt_srgb = linear_to_srgb(np.clip(gt_tm, 0, 1))
        relight_srgb = linear_to_srgb(np.clip(relight_tm, 0, 1))
        
        # Convert to uint8
        gt_u8 = (gt_srgb * 255).astype(np.uint8)
        relight_u8 = (relight_srgb * 255).astype(np.uint8)
        
        # Save ground truth
        cv2.imwrite(os.path.join(test_result_dir, f"gt_{idx:03d}.png"),
                    cv2.cvtColor(gt_u8, cv2.COLOR_RGB2BGR))
        
        # Save relighted image
        cv2.imwrite(os.path.join(test_result_dir, f"relight_{idx:03d}.png"),
                    cv2.cvtColor(relight_u8, cv2.COLOR_RGB2BGR))
        
        # Save side-by-side comparison
        comparison = np.concatenate([gt_u8, relight_u8], axis=1)
        cv2.imwrite(os.path.join(test_result_dir, f"comparison_{idx:03d}.png"),
                    cv2.cvtColor(comparison, cv2.COLOR_RGB2BGR))
        
        # Compute metrics
        psnr_vals.append(compute_psnr(gt_u8, relight_u8))
        ssim_vals.append(compute_ssim(gt_u8, relight_u8))
        lpips_vals.append(compute_lpips(gt_u8, relight_u8))
    
    # Average metrics
    avg_psnr = np.mean(psnr_vals)
    avg_ssim = np.mean(ssim_vals)
    avg_lpips = np.mean(lpips_vals)
    
    print(f"    Test Results: PSNR={avg_psnr:.4f}, SSIM={avg_ssim:.4f}, LPIPS={avg_lpips:.4f}")
    
    # Save metrics
    with open(os.path.join(test_result_dir, "metrics.txt"), "w") as f:
        f.write(f"Object: {obj_name}\n")
        f.write(f"Lighting: {lighting_type}\n")
        f.write(f"Material: {material_name}\n")
        f.write(f"Number of test images: {num_test}\n")
        f.write(f"Average PSNR: {avg_psnr:.4f}\n")
        f.write(f"Average SSIM: {avg_ssim:.4f}\n")
        f.write(f"Average LPIPS: {avg_lpips:.4f}\n\n")
        f.write("Per-image metrics:\n")
        for i in range(num_test):
            f.write(f"  Image {i:03d}: PSNR={psnr_vals[i]:.2f}, SSIM={ssim_vals[i]:.4f}, LPIPS={lpips_vals[i]:.4f}\n")
    
    # Log to global results
    global_log.write(f"{obj_name},{lighting_type},{material_name},{avg_psnr:.4f},{avg_ssim:.4f},{avg_lpips:.4f}\n")
    global_log.flush()
    
    # Clear GPU memory
    torch.cuda.empty_cache()


def main():
    os.makedirs(RESULT_DIR, exist_ok=True)
    
    with open(os.path.join(RESULT_DIR, "all_results.txt"), "w") as global_log:
        global_log.write("Object,Lighting,Material,PSNR,SSIM,LPIPS\n")
        
        for obj in OBJECTS:
            print(f"\n{'#'*70}")
            print(f"# Processing object: {obj}")
            print(f"{'#'*70}")
            
            for lighting in LIGHTING_TYPES:
                # Choose appropriate material list
                materials = MATERIALS_MULTI if lighting == "Multi" else MATERIALS
                
                for material in materials:
                    # Check if path exists
                    check_path = os.path.join(SYNTHRTI_PATH, lighting, obj, material, "Dome")
                    if os.path.isdir(check_path):
                        try:
                            process_material(obj, lighting, material, RESULT_DIR, global_log)
                        except Exception as e:
                            print(f"  ERROR: {e}")
                            import traceback
                            traceback.print_exc()
                            global_log.write(f"{obj},{lighting},{material},ERROR\n")
                            global_log.flush()
    
    print(f"\n{'='*70}")
    print(f"ALL DONE! Results saved to {RESULT_DIR}")
    print(f"{'='*70}")

if __name__ == "__main__":
    main()