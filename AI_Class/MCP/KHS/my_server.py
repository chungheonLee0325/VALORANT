from fastmcp import FastMCP, Client
import asyncio

mcp = FastMCP(name='FirstServer')

@mcp.tool()
def greet(name: str) -> str:
    """간단한 인사 말합니다."""
    return f"안녕하세요, {name}님!"

@mcp.tool()
def add(a: int, b: int) -> int:
    """두 수를 더합니다."""
    return a + b

print("도구 'greet'와 'add'가 추가되었습니다.")

APP_CONFIG = {"theme": "dark", "version": "1.1", "feature_flags": ["new_dashboard"]}

@mcp.resource("data://config")
def get_config() -> dict:
    """애플리케이션 구성을 제공합니다."""
    return APP_CONFIG

print("리소스 'data://config'가 추가되었습니다.")

USER_PROFILES = {
    101: {"name": "앨리스", "status": "active"},
    102: {"name": "밥", "status": "inactive"},
}

@mcp.resource("users://{user_id}/profile")
def get_user_profile(user_id: int) -> dict:
    """사용자의 ID로 사용자 프로필을 검색합니다."""
    # URI에서의 {user_id}가 자동으로 인수로 전달됩니다.
    return USER_PROFILES.get(user_id, {"error": "사용자를 찾을 수 없습니다."})

print("리소스 템플릿 'users://{user_id}/profile'가 추가되었습니다.")

@mcp.prompt("summarize")
async def summarize_prompt(text: str) -> list[dict]:
    """제공된 텍스트를 요약하는 프롬프트를 생성합니다."""
    return [
        {"role": "system", "content": "당신은 요약에 능숙한 유용한 조수입니다."},
        {"role": "user", "content": f"다음 텍스트를 요약해 주세요:\n\n{text}"}
    ]

print("프롬프트 'summarize'가 추가되었습니다.")

async def test_server_locally():
    print("\n--- 로컬 서버 테스트 중 ---")
    # 클라이언트가 서버 객체를 가리키도록 설정합니다.
    client = Client(mcp)

    # 클라이언트는 비동작적이므로 비동기 컨텍스트 관리자를 사용합니다.
    async with client:
        # 'greet' 도구 호출
        greet_result = await client.call_tool("greet", {"name": "FastMCP 사용자"})
        print(f"greet 결과: {greet_result}")

        # 'add' 도구 호출
        add_result = await client.call_tool("add", {"a": 5, "b": 7})
        print(f"add 결과: {add_result}")

        # 'config' 리소스 읽기
        config_data = await client.read_resource("data://config")
        print(f"config 리소스: {config_data}")

        # 템플릿을 사용하여 사용자 프로필 읽기
        user_profile = await client.read_resource("users://101/profile")
        print(f"사용자 101 프로필: {user_profile}")

        # 'summarize' 프롬프트 구조 얻기(여기서 LLM 호출을 실행하지 않음)
        prompt_messages = await client.get_prompt("summarize", {"text": "이것은 일부 텍스트입니다."})
        print(f"요약 프롬프트 구조: {prompt_messages}")

if __name__ == "__main__":
    print("\n--- __main__을 통해 FastMCP 서버 시작 중 ---")
    # 이는 서버를 시작합니다. 일반적으로 기본적으로 stdio 전송을 사용합니다.
    mcp.run()