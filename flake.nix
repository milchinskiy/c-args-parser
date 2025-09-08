{
  description = "c-args-parser project";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = {
    self,
    nixpkgs,
  }: let
    supportedSystems = [
      "x86_64-linux"
      "aarch64-linux"
      "x86_64-darwin"
      "aarch64-darwin"
    ];

    eachSystem = nixpkgs.lib.genAttrs supportedSystems (
      system: let
        pkgs = nixpkgs.legacyPackages.${system};
        nativeBuildInputs = with pkgs; [
          llvmPackages_latest.clang-tools
          llvmPackages_latest.clang
          meson
          ninja
          mesonlsp
        ];
        buildInputs = with pkgs; [
          pkg-config
        ];
      in {
        devShell = pkgs.mkShell {
          inherit nativeBuildInputs buildInputs;
          packages = with pkgs; [
            cmake-language-server
            gdb
          ];
          shellHook = ''
            export CC=clang
            echo "c-args-parser project"
            clang --version
          '';
        };

        package = pkgs.stdenv.mkDerivation {
          inherit nativeBuildInputs buildInputs;
          name = "c-args-parser";
          src = ./.;
          installPhase = ''
            echo "Installing project"
          '';
          buildPhase = ''
            echo "Building project"
          '';
        };
      }
    );
  in {
    devShells =
      nixpkgs.lib.mapAttrs (system: systemAttrs: {
        default = systemAttrs.devShell;
      })
      eachSystem;

    packages =
      nixpkgs.lib.mapAttrs (system: systemAttrs: {
        default = systemAttrs.package;
      })
      eachSystem;
  };
}
