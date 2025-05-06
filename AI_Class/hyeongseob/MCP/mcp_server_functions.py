import numpy as np
import pandas as pd
import koreanize_matplotlib
import matplotlib.pyplot as plt

# 함수 1.
def machinegun_recoil_points(shots: int):
    """
    machinegun_recoil_points(shots: int):
    기관총 반동 궤적을 생성합니다. 
    반동이 강하고 연속적인 사격 패턴을 모사합니다.
    """
    x = []
    y = []

    for i in range(1, shots + 1):
        # 초탄
        if i <= int(shots/3):
            dx = np.random.uniform(0.0, 0.2)        # X축 흔들림 거의 없음
            dy = np.random.uniform(0.0, 0.65)       # Y축 미세 흔들림 증가
        # 중탄
        elif i <= int((shots/3)*2):
            dx = np.random.normal(0.1, 0.3)         # X축 미세 흔들림 증가
            dy = np.random.normal(0.1, 0.3)         # Y축 미세 흔들림 증가
        # 후탄
        else:
            dx = np.random.uniform(-1, 0.5)         # X축 흔들림 강함
            dy = np.random.normal(0.0 , 0.2)        # Y축 흔들림 거의 없음

        x.append(dx)
        y.append(dy)

    # x의 요소를 누적합으로 계산
    x_cum = np.cumsum(x)
    y_cum = np.cumsum(y)
    print("생성이 완료되었습니다")
    
    return x_cum, y_cum

# 함수 2.
def pistol_recoil_points(shots: int):
    """
    pistol_recoil_points(shots: int):
    권총 반동 궤적을 생성합니다. 
    상대적으로 짧고 일관된 반동 패턴을 가집니다.
    """
    x = []
    y = []

    for i in range(1, shots + 1):
        # 초탄
        if i <= int(shots/3):
            dx = np.random.uniform(0.0, 0.2)        # 약간의 X축 흔들림 부여
            dy = np.random.uniform(0.0, 0.5)        # 세로 반동 (조금 줄임)
        # 중탄
        elif i <= int((shots/3)*2):
            dx = np.random.uniform(0.3, 0.3)        # X축 미세 흔들림 증가
            dy = np.random.uniform(0.3, 0.3)
        # 후탄
        else:
            dx = np.random.uniform(-0.1, 0.3)       # X축 흔들림 강함
            dy = np.random.uniform(-0.1, 0.2)         # Y축 거의 없음

        x.append(dx)
        y.append(dy)

    # x의 요소를 누적합으로 계산
    x_cum = np.cumsum(x)
    y_cum = np.cumsum(y)
    print("생성이 완료되었습니다")

    return x_cum, y_cum

# 함수. 3
def shotgun_recoil_points(shots: int):
    """
    shotgun_recoil_points(shots: int):
    산탄총 반동 궤적을 생성합니다.
    퍼짐이 강한 산포형 탄착군을 모사합니다.
    """
    x = []
    y = []

    pellets_per_shot = 16  # 한 발에 퍼지는 산탄 수

    for i in range(1, shots + 1):
        for _ in range(pellets_per_shot):
            if i == 1:
                dx = np.random.normal(0.0, 0.5)
                dy = np.random.normal(0.0, 0.5)
            else:
                dx = np.random.normal(0.0, 2.5)
                dy = np.random.normal(0.0, 2.5)

            x.append(dx)
            y.append(dy)

    # 누적합 없이 그대로 분포 시각화
    x_cum = x
    y_cum = y
    print("생성이 완료되었습니다")

    return x_cum, y_cum
    
# 함수. 4    
def plot_recoil_pattern(data):
    """생성된 좌표값을 시각화하는 메서드"""
    plt.figure(figsize=(5, 5))
    plt.scatter(data[0], data[1], c='red', s=10, marker='s')  # y축 반전해서 위로 튀는 느낌
    plt.axhline(0, color='black', linestyle='--', linewidth=1)
    plt.axvline(0, color='black', linestyle='--', linewidth=1)
    plt.gca().set_facecolor('white')
    plt.grid(True, linestyle=':', color='grey', alpha=0.3)
    plt.xticks(np.arange(-10, 10, 1))
    plt.yticks(np.arange(-10, 10, 1))
    plt.xlim(-10, 10)
    plt.ylim(-10, 10)
    plt.gca().set_aspect('equal')
    plt.title("Improved Recoil Pattern", color='black')
    plt.tick_params(colors='black')
    plt.tight_layout()
    
    return plt.show()

# 함수. 5
def dataset(data):
    """생성된 좌표값을 데이터프레임화하는 메서드"""
    df = pd.DataFrame({"x": data[0], "y": data[1]})
    
    return df