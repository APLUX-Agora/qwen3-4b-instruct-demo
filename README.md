# Qwen3-4B-Instruct-2507

Minimal on-device inference example. [Model resources](https://huggingface.co/APLUX-Agora/Qwen3-4B-instruct-2507) are hosted separately on Hugging Face.

Targets: QCS8550, QCS8625, IQ8, IQ9. The supplied model shards were compiled with QNN 2.36; the minimal example uses the installed AidGen backend (preferring QNN 2.48). Other board combinations need their own runtime validation.

## 1. Check the environment

Download `agora.env.example` as `agora.env`, edit its two values if needed, then load it.
Agora's workspace provides the same file with your selections filled in.

```bash
curl -fL https://raw.githubusercontent.com/APLUX-Agora/qwen3-4b-instruct-demo/main/agora.env.example -o agora.env
source ./agora.env
mkdir -p "$AGORA_WORKSPACE"
curl -fL https://raw.githubusercontent.com/APLUX-Agora/qwen3-4b-instruct-demo/main/tools/check-environment.py \
  -o "$AGORA_WORKSPACE/check-environment.py"
python3 "$AGORA_WORKSPACE/check-environment.py" --soc "$AGORA_SOC" --ubuntu 22.04
```

QCS6490/QCS8550/QCS8625 use Ubuntu 22.04 in this demo; IQ8/IQ9 use Ubuntu 24.04.
`CHECKED` means the expected value matches. Fix `FAILED` before proceeding.
For `NEEDS_SETUP`, install the dependencies below and rerun the check.

## 2. Get the code

```bash
git clone https://github.com/APLUX-Agora/qwen3-4b-instruct-demo.git "$AGORA_WORKSPACE/code"
cd "$AGORA_WORKSPACE/code"
```

Already cloned? Skip this step, or run `git pull --ff-only` to update.

## 3. Set up dependencies

QCS8550 example; installed system packages can be skipped.

```bash
sudo aid-pkg install aidgen-sdk
sudo aid-pkg install aidgen-qnn248
sudo apt-get update
sudo apt-get install -y git cmake g++ python3-venv
cd "$AGORA_WORKSPACE/code"
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
cmake -S . -B build
cmake --build build -j2
```

Keep this terminal open. In a new terminal, source your `agora.env` and activate
`$AGORA_WORKSPACE/code/.venv` before continuing.

## 4. Get model resources

```bash
hf download APLUX-Agora/Qwen3-4B-instruct-2507 \
  --include "targets/$AGORA_SOC/*" --local-dir "$AGORA_WORKSPACE/resources"
```

Public download, no login required. All files for the selected SoC stay together.

## 5. Run a test

```bash
cd "$AGORA_WORKSPACE/code"
./build/qwen3_demo --models "$AGORA_WORKSPACE/resources/targets/$AGORA_SOC" \
  --prompt "What is the capital of France? Answer in one sentence."
```

Expected: A streamed answer followed by `COMPLETED`. The compiled context is 4096. This is a single-turn C++ example using the public AidGen API.

## Validation

QCS8550 / Ubuntu 22.04: tested with AidGen 2.4.4.171 and the QNN 2.48 backend, using the supplied 4096-context model. Other hardware combinations require validation.

## License

Example code: Apache-2.0. Model licenses are documented in the HF repository. AidGen and QNN SDKs are installed separately; no SDK source or binaries are included.
