import cv2
import glob
import numpy as np
#import tifffile
import base64, json
import pickle
from PIL import Image

def load_lightlp(filename=None):
    """
        Load light file specified by filename.
        The format of lights.lp should be
            numbers of images
            path_image_1 light1_x light1_y light1_z
            path_image_2 light2_x light2_y light2_z
            path_image_3 light3_x light3_y light3_z
            ...
            lightf_x lightf_y lightf_z

        :param filename: filename of lights.txt
        """
    if filename is None:
        raise ValueError("filename is None")
    Lt = np.loadtxt(filename, skiprows=1, usecols=(1,2,3))
    return Lt.T

def load_lighttxt(filename=None):
    """
    Load light file specified by filename.
    The format of lights.txt should be
        light1_x light1_y light1_z
        light2_x light2_y light2_z
        ...
        lightf_x lightf_y lightf_z

    :param filename: filename of lights.txt
    :return: light matrix (3 \times f)
    """
    if filename is None:
        raise ValueError("filename is None")
    Lt = np.loadtxt(filename)
    return Lt.T


def load_lightnpy(filename=None):
    """
    Load light numpy array file specified by filename.
    The format of lights.npy should be
        light1_x light1_y light1_z
        light2_x light2_y light2_z
        ...
        lightf_x lightf_y lightf_z

    :param filename: filename of lights.npy
    :return: light matrix (3 \times f)
    """
    if filename is None:
        raise ValueError("filename is None")
    Lt = np.load(filename)
    return Lt.T


def load_image(filename=None):
    """
    Load image specified by filename (read as a gray-scale)
    :param filename: filename of the image to be loaded
    :return img: loaded image
    """
    if filename is None:
        raise ValueError("filename is None")
    return cv2.imread(filename, 0)

def load_image_list(imagelist, crop):

#sudo apt install libturbojpeg-dev
    M = None
    height = 0
    width = 0

    if crop is not None:
        y = crop['y']
        x = crop['x']
        w = crop['width']
        h = crop['height']

    count = 1
    for fname in imagelist:
        print("Loading images: " + str(int(100*count/len(imagelist))) + "%", flush=True)
        count += 1
        im = cv2.imread(fname).astype(np.float64)

        if im.ndim == 3:
            # Assuming that RGBA will not be an input
            im = np.mean(im, axis=2)   # RGB -> Gray

        if crop is not None:
            im = im[y:y + crop['height'], crop['x']:crop['x'] + crop['width']]

        if M is None:
            height, width = im.shape
            M = im.reshape((-1, 1))
        else:
            M = np.append(M, im.reshape((-1, 1)), axis=1)
    return M, height, width


def load_images(foldername=None, ext=None):
    """
    Load images in the folder specified by the "foldername" that have extension "ext"
    :param foldername: foldername
    :param ext: file extension
    :return: measurement matrix (numpy array) whose column vector corresponds to an image (p \times f)
    """
    if foldername is None or ext is None:
        raise ValueError("filename/ext is None")

    return load_image_list(sorted(glob.glob(foldername + "*." + ext)))


def load_npyimages(foldername=None):
    """
    Load images in the folder specified by the "foldername" in the numpy format
    :param foldername: foldername
    :return: measurement matrix (numpy array) whose column vector corresponds to an image (p \times f)
    """
    if foldername is None:
        raise ValueError("filename is None")

    M = None
    height = 0
    width = 0
    for fname in sorted(glob.glob(foldername + "*.npy")):
        im = np.load(fname)
        if im.ndim == 3:
            im = np.mean(im, axis=2)
        if M is None:
            height, width = im.shape
            M = im.reshape((-1, 1))
        else:
            M = np.append(M, im.reshape((-1, 1)), axis=1)
    return M, height, width


def disp_normalmap(normal=None, height=None, width=None, delay=0, name=None):
    """
    Visualize normal as a normal map
    :param normal: array of surface normal (p \times 3)
    :param height: height of the image (scalar)
    :param width: width of the image (scalar)
    :param delay: duration (ms) for visualizing normal map. 0 for displaying infinitely until a key is pressed.
    :param name: display name
    :return: None
    """
    if normal is None:
        raise ValueError("Surface normal `normal` is None")
    N = np.reshape(normal, (height, width, 3))  # Reshape to image coordinates
    N[:, :, 0], N[:, :, 2] = N[:, :, 2], N[:, :, 0].copy()  # Swap RGB <-> BGR
    N = (N + 1.0) / 2.0  # Rescale
    if name is None:
        name = 'normal map'
    cv2.namedWindow(name, cv2.WINDOW_NORMAL);
    im_s = cv2.resize(N, (960, 540))
    cv2.imshow(name, im_s)
    cv2.waitKey(delay)
    cv2.destroyWindow(name)
    cv2.waitKey(1)    # to deal with frozen window...


def save_normalmap_as_npy(filename=None, normal=None, height=None, width=None):
    """
    Save surface normal array as a numpy array
    :param filename: filename of the normal array
    :param normal: surface normal array (height \times width \times 3)
    :return: None
    """
    if filename is None:
        raise ValueError("filename is None")
    N = np.reshape(normal, (height, width, 3))
    N = (N + 1.0) / 2.0  # Rescale
    np.save(filename, N)

def save_normalmap_as_tiff(filename=None, normal=None, height=None, width=None):
    """
       Save surface normal array as a numpy array
       :param filename: filename of the normal array
       :param normal: surface normal array (height \times width \times 3)
       :return: None
       """
    if filename is None:
        raise ValueError("filename is None")
    N = np.reshape(normal, (height, width, 3))  # Reshape to image coordinates
    N[:, :, 0], N[:, :, 2] = N[:, :, 2], N[:, :, 0].copy()  # Swap RGB <-> BGR
    N = (N + 1.0) / 2.0  # Rescale
    M = np.float32(N)   #convert from float64 to float32
    cv2.imwrite(filename, M)

def save_normalmap_as_png(filename=None, normal=None, height=None, width=None):
    """
       Save surface normal array as a numpy array
       :param filename: filename of the normal array
       :param normal: surface normal array (height \times width \times 3)
       :return: None
       """
    if filename is None:
        raise ValueError("filename is None")
    N = np.reshape(normal, (height, width, 3))  # Reshape to image coordinates
    #N[:, :, 0], N[:, :, 2] = N[:, :, 2], N[:, :, 0].copy()  # Swap RGB <-> BGR
    N = (N + 1.0) / 2.0  # Rescale
    #convert from float64 to uint16
    N *= 65535
    M = N.astype(np.uint16)
    #saving file
    cv2.imwrite(filename, M)

def save_normalmap_as_jpg(filename=None, normal=None, height=None, width=None):
    """
        Save surface normal array as a numpy array
       :param filename: filename of the normal array
       :param normal: surface normal array (height \times width \times 3)
       :return: None
       """
    if filename is None:
        raise ValueError("filename is None")
    N = np.reshape(normal, (height, width, 3))  # Reshape to image coordinates
    #N[:, :, 0], N[:, :, 2] = N[:, :, 2], N[:, :, 0].copy()  # Swap RGB <-> BGR
    N = (N + 1.0) / 2.0  # Rescale
    # convert from float64 to 8bit bit image
    N *= 255
    M = N.astype(int)
    # saving file
    cv2.imwrite(filename, M)
def save_normalmap_as_json(filename=None, normal=None, height=None, width=None):
    """
       Save surface normal array as a numpy array
       :param filename: filename of the normal array
       :param normal: surface normal array (height \times width \times 3)
       :return: None
       """
    '''
    if filename is None:
        raise ValueError("filename is None")
    N = np.reshape(normal, (height, width, 3))
    _, imdata = cv2.imencode('.jpg', N)
    jstr = json.dumps({"image": base64.b64encode(imdata).decode('ascii')})
    print(jstr)
    return jstr
    '''



def load_normalmap_from_npy(filename=None):
    """
    Load surface normal array (which is a numpy array)
    :param filename: filename of the normal array
    :return: surface normal (numpy array) in formatted in (height, width, 3).
    """
    if filename is None:
        raise ValueError("filename is None")
        raise ValueError("filename is None")
    return np.load(filename)



def evaluate_angular_error(gtnormal=None, normal=None, background=None):
    if gtnormal is None or normal is None:
        raise ValueError("surface normal is not given")
    ae = np.multiply(gtnormal, normal)
    aesum = np.sum(ae, axis=1)
    coord = np.where(aesum > 1.0)
    aesum[coord] = 1.0
    coord = np.where(aesum < -1.0)
    aesum[coord] = -1.0
    ae = np.arccos(aesum) * 180.0 / np.pi
    if background is not None:
        ae[background] = 0
    return ae


