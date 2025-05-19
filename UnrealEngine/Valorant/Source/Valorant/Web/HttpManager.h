// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Interfaces/IHttpRequest.h"
#include "UObject/Object.h"
#include "HttpManager.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHttpManager, Log, All);

#define HTTP_LOG(Format, ...) \
	{ \
		SET_WARN_COLOR(COLOR_GREEN); \
		UE_LOG(LogHttpManager, Log, TEXT("%s"), *FString::Printf(Format, ##__VA_ARGS__)); \
		CLEAR_WARN_COLOR(); \
	}

USTRUCT(BlueprintType)
struct FHttpQuery
{
	GENERATED_BODY()
	
	TMap<FString, FString> QueryMap;
	
	void AddQuery(const FString& Key, const FString& Value)
	{
		QueryMap.Add(Key, Value);
	}
	
	FString ToQueryString() const
	{
		FString QueryString;
		for (const auto& Pair : QueryMap)
		{
			if (!QueryString.IsEmpty()) QueryString += TEXT("&");
			QueryString += FGenericPlatformHttp::UrlEncode(Pair.Key) + "=" + FGenericPlatformHttp::UrlEncode(Pair.Value);
		}
		return QueryString;
	}

	FString AppendToURL(const FString& BaseURL) const
	{
		const FString QueryString = ToQueryString();
		if (QueryString.IsEmpty())
		{
			return BaseURL;
		}
		if (BaseURL.Contains(TEXT("?")))
		{
			return BaseURL + TEXT("&") + QueryString;
		}
		return BaseURL + TEXT("?") + QueryString;
	}
};

/**
 * 
 */
UCLASS()
class VALORANT_API UHttpManager : public UObject
{
	GENERATED_BODY()

public:
	static UHttpManager* GetInstance();
	void SendRequest(const FString& URL, const FString& Verb = TEXT("GET"), const FString& Content = TEXT(""), const TFunction<void(FHttpResponsePtr, bool)>& Callback = TFunction<void(FHttpResponsePtr, bool)>());

private:
	static UHttpManager* Singleton;
	TMap<FHttpRequestPtr, TFunction<void(FHttpResponsePtr, bool)>> PendingRequests;
	
	void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};