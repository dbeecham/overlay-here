{

  inputs = {
    # updated 2021-12-01
    nixpkgs = {
      type = "github";
      owner = "NixOS";
      repo = "nixpkgs";
      rev = "c30bbcfae7a5cbe44ba4311e51d3ce24b5c84e1b";
    };
  };


  outputs = { self, nixpkgs }: {

    defaultPackage.x86_64-linux = nixpkgs.legacyPackages.x86_64-linux.stdenv.mkDerivation {
      name = "overlay-here";
      srcs = [
        ./Makefile
        ./Kconfig
        ./src
      ];
      unpackPhase = ''
        for file in $srcs; do
          cp -r $file $(stripHash $file)
        done
      '';
      installFlags = [ "DESTDIR=$(out)" "PREFIX=/" ];
    };

    nixosModule = { pkgs, config, ...}: {
      security.wrappers.overlay-here = {
        setuid = true;
        owner = "root";
        group = "nogroup";
        source = "${self.defaultPackage.x86_64-linux}/bin/overlay-here";
      };
    };

  };

}
