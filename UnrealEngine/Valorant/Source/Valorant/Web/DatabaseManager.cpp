// Fill out your copyright notice in the Description page of Project Settings.

#include "DatabaseManager.h"

void UDatabaseManager::FetchOrCreateUser()
{
	const FString URL = FString::Printf(TEXT("https://g10b8556d063956-valorithm.adb.ap-seoul-1.oraclecloudapps.com/ords/admin/user/%d"), 1);
	SendRequest(URL, "GET");
}

void UDatabaseManager::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
}
