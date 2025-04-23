from fastapi import APIRouter
from pydantic import BaseModel
from Server.utils import generate_segmented_recoil

recoil_router = APIRouter(prefix="/recoil", tags=["Weapon Recoil Segemented Data"])

class Data(BaseModel):
    data: int

@recoil_router.post("/generation")
def recoil_generation(data: Data):
    data = generate_segmented_recoil(30)
    try:
        return data
    except Exception as e:
        print("오류 발생:", e)