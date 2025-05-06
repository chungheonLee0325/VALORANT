from fastmcp.client import Client

client = Client(
    base_url = "http://0.0.0.0/8005")




async def main():
    # FastMCP 클라이언트 생성
    
    # 컨텍스트 매니저 사용
    async with client:
        try:
            # 사용 가능한 도구 목록 확인
            tools = await client.list_tools()
            print("연결 성공! 사용 가능한 도구:", tools)
            
            # 예시: 기관총 반동 10발 생성
            result = await client.call_tool("machinegun_recoil_points", {"shots": 10})
            print("반동 생성 결과:", result)
            
            # 결과 시각화
            await client.call_tool("plot_recoil_pattern", {"data": result})
            
        except Exception as e:
            print(f"작업 실패: {e}")

# 비동기 함수 실행
if __name__ == "__main__":
    asyncio.run(main())