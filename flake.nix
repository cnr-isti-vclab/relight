{
    inputs = {
        flake-utils.url = "github:numtide/flake-utils";
    };

  outputs = { self, nixpkgs, flake-utils }: {
    overlays.default = final: prev: {
      relightlab = final.callPackage ({stdenv, pkgs, ...}:
        stdenv.mkDerivation {
          src = ./.;
          pname = "RelightLab";
          version = "2026.09";

          nativeBuildInputs = with final; [
              cmake
              ninja
              patchelf
              qt6.wrapQtAppsHook
              pkg-config
          ];

          buildInputs = with final; [
              mesa
              libGLU
              fuse
              libjpeg
              libtiff
              eigen
              libxcb-cursor
              lcms2
              qt6.qtbase
              qt6.qtsvg
              exiv2
              opencv4
          ];
        }
      ) {};
    };
  } // 
  (flake-utils.lib.eachDefaultSystem (system:
    let
      pkgs = import nixpkgs {
        system = system;
        overlays = [ self.overlays.default ];
      };
    in
      {
        packages = {
          relightlab = pkgs.relightlab;
          default = self.packages.${system}.relightlab;
        };
      }
  ));
}
