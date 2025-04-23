import numpy as np
import pandas as pd
import json
import matplotlib.pyplot as plt
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestRegressor
from sklearn.metrics import mean_absolute_error

# 1. Scikit-Learn 기반 총알 궤적 생성 Class
class BulletTrajectory:
    """총알 궤적을 생성하고 전처리 및 분리하는 클래스"""
    def __init__(self):
        self.data = None
        self.X_coords = None
        self.y_coords = None

    def data_load(self, address: str):
        """데이터 로드 함수"""
        with open(address, "r", encoding="utf-8") as f:
            self.data = json.load(f)

        return self.data

    def data_preprocessing(self, raw_data):
        """데이터 전처리 함수"""
        X_coords, y_coords = [], []
        for record in raw_data:
            record_points = record["DATA"]
            X_coords.append(np.array([[point['x']] for point in record_points]))
            y_coords.append(np.array([[point['y']] for point in record_points]))
        self.X_coords = X_coords
        self.y_coords = y_coords

        return X_coords, y_coords

    def weapon_recoil_generation(self, X_coords, y_coords):
        """데이터 프레임 생성하는 함수"""
        df1 = pd.DataFrame(np.array(X_coords).reshape(100, -1))
        df2 = pd.DataFrame(np.array(y_coords).reshape(100, -1))
        X1 = df1.drop(columns=[0])
        y1 = df1[0]
        X2 = df2.drop(columns=[0])
        y2 = df2[0]

        return X1, y1, X2, y2

    def data_split(self, x, y):
        """학습 데이터와 테스트 데이터를 분리하는 함수"""
        X_train, X_test, y_train, y_test = train_test_split(x, y, test_size=0.2, random_state=42)
        X_train, X_val, y_train, y_val = train_test_split(X_train, y_train, test_size=0.2, random_state=42)
        
        return X_train, X_test, X_val, y_train, y_test, y_val


# 2. Train Dataset 생성 Class
def generate_segmented_recoil(shots):
    """입력값 기반 좌표값 데이터 생성 함수"""
    x = []
    y = []

    for i in range(1, shots + 1):
        # 초탄
        if i <= int(shots/3):
            dx = np.random.normal(0.1, 0.05)        # 약간의 X축 흔들림 부여
            dy = np.random.uniform(0.3, 0.5)        # 세로 반동 (조금 줄임)
        # 중탄
        elif i <= int((shots/3)*2):
            dx = np.random.normal(0.3, 0.25)        # X축 미세 흔들림 증가
            dy = np.random.normal(0.3, 0.3)
        # 후탄
        else:
            dx = np.random.uniform(-1, 1)           # X축 흔들림 강함
            dy = np.random.normal(0.05, 0.02)       # Y축 거의 없음

        x.append(dx)
        y.append(dy)

    x_cum = np.cumsum(x)
    y_cum = np.cumsum(y)

    return x_cum, y_cum


def generate_recoil_dataset(shots):
    """생성된 좌표값을 Json Type File로 저장하는 함수"""
    recoil_dataset = []
    for i in range(100):
        x, y = generate_segmented_recoil(shots)
        dataset = [{'x': float(xi), 'y': float(yi)} for xi, yi in zip(x, y)]
        recoil_dataset.append({
            f'INDEX': i,
            'DATA': dataset
        })

    with open ("data/recoil_segmented_dataset.json", "w", encoding="utf-8") as f:
        json.dump(recoil_dataset, f, indent=2, ensure_ascii=False)

    print("'recoil_segmented_dataset.json' 파일로 저장 완료되었습니다.")

    return x, y


def plot_recoil_pattern(x, y):
    """생성된 좌표값을 시각화하는 함수"""
    plt.figure(figsize=(5, 5))
    plt.scatter(x, y, c='red', s=10, marker='s')  # y축 반전해서 위로 튀는 느낌
    plt.axhline(0, color='black', linestyle='--', linewidth=1)
    plt.axvline(0, color='black', linestyle='--', linewidth=1)
    plt.gca().set_facecolor('white')
    plt.grid(True, linestyle=':', color='grey', alpha=0.3)
    plt.xticks(np.arange(-10, 10, 1))
    plt.yticks(np.arange(-10, 10, 1))
    plt.xlim(-10, 10)
    plt.ylim(-10, 10)
    plt.gca().set_aspect('equal')
    plt.title("Improved Recoil Pattern (30 shots)", color='black')
    plt.tick_params(colors='black')
    plt.tight_layout()
    plt.show()