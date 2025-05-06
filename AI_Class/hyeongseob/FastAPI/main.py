from fastapi import FastAPI, APIRouter

test_router = APIRouter(prefix="/test", tags=["Test"])

app = FastAPI(
    title="VALORITHM",
    description="게임 개발자를 위한 AI 보조 도구 기반 FPS 게임 제작 프로젝트",
    version="250428_v1"
)

@test_router.get("/v1")
def generator(input: str):
    return(f"{input} 입력이 완료되었습니다")

@test_router.post("v1/post")
def post_generator(data: str):
    return(f"{data} 출력이 완료되었습니다")

app.include_router(test_router)