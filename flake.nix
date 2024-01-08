{
  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs.url = "github:nixos/nixpkgs/nixos-23.11";
  };
  outputs = { self, nixpkgs, flake-utils }: flake-utils.lib.eachDefaultSystem (system:
    let
      pkgs = import nixpkgs {
        inherit system;
        config.replaceStdenv = { pkgs, ... }: pkgs.gcc11Stdenv;
      };
    in
    {
      devShells.default = pkgs.mkShell {
        nativeBuildInputs = with pkgs; [
          gnumake42
          stdenv
          pkg-config
          valgrind

          openmpi
          python3
          eigen
          libxml2
          boost
          wget
          unzip

          # required for building petsc
          bison
          cmake
          flex
          blas
          lapack
          arpack

          # require for building custom python
          zlib
          bzip2
          libffiBoot
          expat
          xz
          libxcrypt
          openssl

          # docs
          doxygen
          graphviz

          # dev env
          clang-tools_14
          llvm_14
          llvmPackages_14.openmp
          cppcheck
        ];

        MPI_HOME = pkgs.openmpi;
        MPI_IGNORE_MPICC = "TRUE";
      };
    }
  );
}
