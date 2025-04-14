import subprocess
import argparse
import os
import shutil

def run_tripo(input_image: str, output_dir: str):
    """TripoSR 실행"""
    os.makedirs(output_dir, exist_ok=True)
    
    result = subprocess.run([
        "python", "inference.py",  # TripoSR 메인 실행 파일
        "--config", "config.yaml",
        "--input", input_image,
        "--output_dir", output_dir
    ], capture_output=True, text=True)

    print("▶ TripoSR STDOUT:", result.stdout)
    print("▶ TripoSR STDERR:", result.stderr)

    obj_path = os.path.join(output_dir, "mesh.obj")
    return obj_path

def convert_to_fbx(obj_path: str, fbx_path: str):
    """(선택) .obj → .fbx 변환 (예: Blender 사용 시)"""
    # Blender CLI 사용 예시 (미설치 시 건너뛰기)
    subprocess.run([
        "blender", "--background", "--python", "obj2fbx.py", "--", obj_path, fbx_path
    ])

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True, help="Input image path")
    parser.add_argument("--output", default="./output_models", help="Output folder")
    parser.add_argument("--export_format", choices=["obj", "fbx"], default="obj")
    args = parser.parse_args()

    # 1. TripoSR 실행
    obj_path = run_tripo(args.input, args.output)

    # 2. 필요 시 fbx 변환
    if args.export_format == "fbx":
        fbx_path = obj_path.replace(".obj", ".fbx")
        convert_to_fbx(obj_path, fbx_path)
        print("✅ FBX 파일 생성:", fbx_path)
    else:
        print("✅ OBJ 파일 생성:", obj_path)

if __name__ == "__main__":
    main()
