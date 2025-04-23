from fastapi import FastAPI
from AI_Class.Server.recoil import recoil_router
from AI_Class.Server.report import report_router

app = FastAPI(
    title="VALORITHM",
    description="게임 개발자를 위한 AI 보조 도구 기반 FPS 게임 제작 프로젝트",
    version="250423_v1"
)

app.include_router(recoil_router)
app.include_router(report_router)