from fastapi import APIRouter
from Server.utils import WeaponRecoilGeneration, RecoilVisualizer

wsg_router = APIRouter(prefix="/wsg", tags=["Weapon Recoil Generation"])
rvz_router = APIRouter(prefix="/rvz", tags=["Recoil Visualizer"])

wsg = WeaponRecoilGeneration()
rvz = RecoilVisualizer()

@recoil_router.post("/generation")
def recoil_generation(data):
    data = generate_segmented_recoil(30)
    try:
        return data
    except Exception as e:
        print("오류 발생:", e)