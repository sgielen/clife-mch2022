let
  pkgs = (builtins.getFlake "github:yorickvp/esp-idf.nix/main").legacyPackages.${builtins.currentSystem};
  inherit (pkgs) esp-idf esptool;

in pkgs.stdenvNoCC.mkDerivation rec {
  pname = "mch2022-template-app";
  src = ./.;
  version = "0.0.1";
  buildInputs = with pkgs; [ cmake ninja esp-idf ];
  IDF_PATH = "${esp-idf}";
  IDF_TOOLS_PATH = "${esp-idf}/tool";
  # stdenvNoCC is setting $AR to an empty string, confusing cmake
  AR = "xtensa-esp32-elf-ar";

  phases = "unpackPhase buildPhase installPhase fixupPhase";

  dontStrip = true;

  buildPhase = ''
    echo '${version}' > version.txt
    source ${esp-idf}/export.sh
    idf.py build
  '';

  shellHook = ''
    export IDF_TOOLS_PATH=$IDF_PATH/tool
    source $IDF_PATH/export.sh
  '';
}
