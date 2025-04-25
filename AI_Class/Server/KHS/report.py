from fastapi import APIRouter
from pydantic import BaseModel

report_router = APIRouter(prefix="/report", tags=["AI REPORT"])

class Data(BaseModel):
    data: str

@report_router.get("/generation")
def report_print_out(data: str):
    report = data
    try:
        return print(report)
    except Exception as e:
        print("오류 발생:", e)