// Fill out your copyright notice in the Description page of Project Settings.


#include "DatabaseManager.h"

#include "HttpManager.h"
#include "JsonObjectConverter.h"
#include "Interfaces/IHttpResponse.h"

UDatabaseManager* UDatabaseManager::Singleton = nullptr;
FString UDatabaseManager::DatabaseUrl = "";

/* static */ UDatabaseManager* UDatabaseManager::GetInstance()
{
	if (!Singleton)
	{
		Singleton = NewObject<UDatabaseManager>();
		Singleton->AddToRoot();
		GConfig->GetString(TEXT("APIServerUrl"), TEXT("Url"), DatabaseUrl, *FConfigCacheIni::NormalizeConfigIniPath(FPaths::ProjectConfigDir() / TEXT("Secret.ini")));
	}
	return Singleton;
}

void UDatabaseManager::GetPlayer(const FString& PlayerId, const FString& Platform, const FOnGetPlayerCompleted& Callback)
{
	TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField(TEXT("player_id"), PlayerId);
	Json->SetStringField(TEXT("platform"), Platform);
	FString q;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&q);
	FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);
	UE_LOG(LogTemp, Warning, TEXT("q: %s"), *q);

	FHttpQuery Query;
	Query.AddQuery("q", q);
	const FString& Url = Query.AppendToURL(DatabaseUrl + TEXT("player/"));
	UE_LOG(LogTemp, Warning, TEXT("Url: %s"), *Url);
	UHttpManager::GetInstance()->SendRequest(Url, TEXT("GET"), TEXT(""),
		[=](FHttpResponsePtr Response, bool bSuccess)
		{
			FPlayerDTO PlayerDto;
			
			if (!bSuccess || !Response.IsValid())
			{
				Callback.Broadcast(false, PlayerDto);
				return;
			}

			TSharedPtr<FJsonObject> RootJson;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
			if (FJsonSerializer::Deserialize(Reader, RootJson) && RootJson.IsValid())
			{
				const TArray<TSharedPtr<FJsonValue>>* Items;
				if (RootJson->TryGetArrayField(TEXT("items"), Items) && Items->Num() > 0)
				{
					TSharedPtr<FJsonObject> PlayerObj = (*Items)[0]->AsObject();
					if (FJsonObjectConverter::JsonObjectToUStruct(PlayerObj.ToSharedRef(), &PlayerDto))
					{
						Callback.Broadcast(true, PlayerDto);
						return;
					}
				}
			}
			UE_LOG(LogTemp, Error, TEXT("파싱 실패 또는 items[0] 없음"));
			Callback.Broadcast(false, PlayerDto);
		}
	);
}

void UDatabaseManager::PostPlayer(const FString& PlayerId, const FString& Platform)
{
	FPlayerDTO PlayerDto;
	PlayerDto.player_id = PlayerId;
	PlayerDto.platform = Platform;
	FString JsonString;
	FJsonObjectConverter::UStructToJsonObjectString(PlayerDto, JsonString);
	UHttpManager::GetInstance()->SendRequest(DatabaseUrl + TEXT("player/"), TEXT("POST"), JsonString);
}