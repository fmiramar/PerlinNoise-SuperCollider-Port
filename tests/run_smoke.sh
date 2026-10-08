#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "${script_dir}/.." && pwd)"

build_dir="${1:-${root}/build}"
extension="${2:?Usage: run_smoke.sh BUILD_DIR INSTALLED_PERLINNOISE_DIR}"
if [[ -d /Applications/SuperCollider.app/Contents/Resources ]]; then
    sc_resources=/Applications/SuperCollider.app/Contents/Resources
    sc_classlib="${SC_CLASSLIB:-${sc_resources}/SCClassLibrary}"
    app_plugins="${SC_PLUGIN_DIR:-${sc_resources}/plugins}"
else
    sc_classlib="${SC_CLASSLIB:-/usr/share/SuperCollider/SCClassLibrary}"
    app_plugins="${SC_PLUGIN_DIR:-/usr/lib/SuperCollider/plugins}"
fi
mkdir -p "${build_dir}/verification"
build_dir="$(cd "${build_dir}" && pwd)"
extension="$(cd "${extension}" && pwd)"
verify_dir="${build_dir}/verification"
osc_path="${verify_dir}/smoke-perlin-noise.osc"
language=(sclang -D -u 0 -a --include-path "${sc_classlib}" --include-path "${extension}")

check_log() {
    if rg -i 'WARNING|ERROR|FAIL|exception' "$1"; then
        echo "PERLIN_NOISE_VERIFICATION_FAILED $1" >&2
        exit 1
    fi
}

"${language[@]}" "${root}/tests/sc_smoke_perlin_noise.scd" "${osc_path}" 2>&1 | tee "${verify_dir}/score.log"
check_log "${verify_dir}/score.log"

for sample_rate in 44100 48000 96000; do
    out_path="${verify_dir}/smoke-perlin-noise-${sample_rate}.wav"
    scsynth -N "${osc_path}" _ "${out_path}" "${sample_rate}" WAV float \
        -i 0 -o 13 -D 0 -U "${extension}:${app_plugins}" -V 0 \
        2>&1 | tee "${verify_dir}/render-${sample_rate}.log"
    check_log "${verify_dir}/render-${sample_rate}.log"
    "${language[@]}" "${root}/tests/verify_audio.scd" "${out_path}" \
        2>&1 | tee "${verify_dir}/audio-${sample_rate}.log"
    check_log "${verify_dir}/audio-${sample_rate}.log"
    rg -q 'PERLIN_NOISE_AUDIO_OK' "${verify_dir}/audio-${sample_rate}.log"
done

"${language[@]}" "${root}/tests/verify_scdoc.scd" "${verify_dir}/Help" \
    2>&1 | tee "${verify_dir}/scdoc.log"
check_log "${verify_dir}/scdoc.log"
rg -q 'PERLIN_NOISE_SCDOC_OK' "${verify_dir}/scdoc.log"
echo "PERLIN_NOISE_SMOKE_OK ${verify_dir}"
