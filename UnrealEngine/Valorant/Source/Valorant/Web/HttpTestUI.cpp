// Fill out your copyright notice in the Description page of Project Settings.


#include "HttpTestUI.h"

#include "HttpManager.h"

void UHttpTestUI::OnHttpResponseReceived(int32 ResponseCode, const FString& ResponseBody)
{
	
}

void UHttpTestUI::SendRequest()
{
	auto* HttpManager = UHttpManager::GetInstance();
	HttpManager->OnHttpResponseReceived.AddDynamic(this, &UHttpTestUI::OnHttpResponseReceived);

	FString Url = "http://192.168.10.90:8080/test/v1";
	FHttpQuery Query;
	Query.AddQuery("input", "9999");
	// Query.AddQuery("test", "wow");
	Url = Query.AppendToURL(Url);
	HttpManager->SendRequest(Url);
	
/*
	LogHttpManager: UHttpManager::SendRequest Called, HTTP Request Info:
		URL: 127.0.0.1:8000/report/generation?damage=3500&test=wow
		Verb: GET
		Header: Content-Type: application/json
		Body: 
 */
}
