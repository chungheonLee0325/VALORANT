#include "HttpManager.h"
#include "Http.h"

DEFINE_LOG_CATEGORY(LogHttpManager);

UHttpManager* UHttpManager::Singleton = nullptr;

/* static */ UHttpManager* UHttpManager::GetInstance()
{
	if (!Singleton)
	{
		Singleton = NewObject<UHttpManager>();
		Singleton->AddToRoot(); // GC 방지
	}
	return Singleton;
}

void UHttpManager::SendRequest(const FString& URL, const FString& Verb, const FString& Content, const TFunction<void(FHttpResponsePtr Response, bool bWasSuccessful)>& Callback)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(URL);
	Request->SetVerb(Verb);
	Request->SetHeader(TEXT("accept"), TEXT("application/json"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	
	if (!Content.IsEmpty())
	{
		Request->SetContentAsString(Content);
	}

	
	// Debugging
	{
		FString DebugString;
		DebugString += FString::Printf(TEXT("\tURL: %s\n"), *Request->GetURL());
		DebugString += FString::Printf(TEXT("\tVerb: %s\n"), *Request->GetVerb());

		// Header
		TArray<FString> HeaderNames = Request->GetAllHeaders();
		for (const FString& Header : HeaderNames)
		{
			DebugString += FString::Printf(TEXT("\tHeader: %s\n"), *Header);
		}

		// Body
		TArray<uint8> ContentBytes = Request->GetContent();
		FString BodyAsString = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(ContentBytes.GetData())));
		DebugString += FString::Printf(TEXT("\tBody: %s\n"), *BodyAsString);
	
		HTTP_LOG(TEXT("%hs Called, HTTP Request Info:\n%s"), __FUNCTION__, *DebugString);
	}
	
	Request->OnProcessRequestComplete().BindUObject(this, &UHttpManager::OnResponseReceived);
	if (Callback)
	{
		PendingRequests.Add(Request, Callback);
	}
	Request->ProcessRequest();
}

void UHttpManager::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	HTTP_LOG(TEXT("%hs Called, bWasSuccessful: %hs"), __FUNCTION__, bWasSuccessful?"Success":"Failed");

	if (PendingRequests.Contains(Request))
	{
		TFunction<void(FHttpResponsePtr, bool)> Callback = PendingRequests[Request];
		PendingRequests.Remove(Request);
		Callback(Response, bWasSuccessful);
	}
}