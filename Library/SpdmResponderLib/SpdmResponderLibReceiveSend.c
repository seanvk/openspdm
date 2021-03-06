/** @file
  SPDM common library.
  It follows the SPDM Specification.

Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SpdmResponderLibInternal.h"

typedef struct {
  UINT8                            RequestResponseCode;
  SPDM_GET_SPDM_RESPONSE_FUNC      GetResponseFunc;
} SPDM_GET_RESPONSE_STRUCT;

SPDM_GET_RESPONSE_STRUCT  mSpdmGetResponseStruct[] = {
  {SPDM_GET_VERSION,                    SpdmGetResponseVersion},
  {SPDM_GET_CAPABILITIES,               SpdmGetResponseCapability},
  {SPDM_NEGOTIATE_ALGORITHMS,           SpdmGetResponseAlgorithm},
  {SPDM_GET_DIGESTS,                    SpdmGetResponseDigest},
  {SPDM_GET_CERTIFICATE,                SpdmGetResponseCertificate},
  {SPDM_CHALLENGE,                      SpdmGetResponseChallengeAuth},
  {SPDM_GET_MEASUREMENTS,               SpdmGetResponseMeasurement},
  {SPDM_KEY_EXCHANGE,                   SpdmGetResponseKeyExchange},
  {SPDM_PSK_EXCHANGE,                   SpdmGetResponsePskExchange},
  {SPDM_GET_ENCAPSULATED_REQUEST,       SpdmGetResponseEncapsulatedRequest},
  {SPDM_DELIVER_ENCAPSULATED_RESPONSE,  SpdmGetResponseEncapsulatedResponseAck},
  {SPDM_RESPOND_IF_READY,               SpdmGetResponseRespondIfReady},

  {SPDM_FINISH,                         SpdmGetResponseFinish},
  {SPDM_PSK_FINISH,                     SpdmGetResponsePskFinish},
  {SPDM_END_SESSION,                    SpdmGetResponseEndSession},
  {SPDM_HEARTBEAT,                      SpdmGetResponseHeartbeat},
  {SPDM_KEY_UPDATE,                     SpdmGetResponseKeyUpdate},
};

/**
  Return the GET_SPDM_RESPONSE function via request code.

  @param  RequestCode                  The SPDM request code.

  @return GET_SPDM_RESPONSE function according to the request code.
**/
SPDM_GET_SPDM_RESPONSE_FUNC
SpdmGetResponseFuncViaRequestCode (
  IN     UINT8                    RequestCode
  )
{
  UINTN                Index;

  ASSERT(RequestCode != SPDM_RESPOND_IF_READY);
  for (Index = 0; Index < sizeof(mSpdmGetResponseStruct)/sizeof(mSpdmGetResponseStruct[0]); Index++) {
    if (RequestCode == mSpdmGetResponseStruct[Index].RequestResponseCode) {
      return mSpdmGetResponseStruct[Index].GetResponseFunc;
    }
  }
  return NULL;
}

/**
  Return the GET_SPDM_RESPONSE function via last request.

  @param  SpdmContext                  The SPDM context for the device.

  @return GET_SPDM_RESPONSE function according to the last request.
**/
SPDM_GET_SPDM_RESPONSE_FUNC
SpdmGetResponseFuncViaLastRequest (
  IN     SPDM_DEVICE_CONTEXT     *SpdmContext
  )
{
  SPDM_MESSAGE_HEADER  *SpdmRequest;

  SpdmRequest = (VOID *)SpdmContext->LastSpdmRequest;
  return SpdmGetResponseFuncViaRequestCode (SpdmRequest->RequestResponseCode);
}

/**
  Process a SPDM request from a device.

  @param  SpdmContext                  The SPDM context for the device.
  @param  SessionId                    Indicate if the request is a secured message.
                                       If SessionId is NULL, it is a normal message.
                                       If SessionId is NOT NULL, it is a secured message.
  @param  IsAppMessage                 Indicates if it is an APP message or SPDM message.
  @param  RequestSize                  Size in bytes of the request data buffer.
  @param  Request                      A pointer to a destination buffer to store the request.
                                       The caller is responsible for having
                                       either implicit or explicit ownership of the buffer.

  @retval RETURN_SUCCESS               The SPDM request is received successfully.
  @retval RETURN_DEVICE_ERROR          A device error occurs when the SPDM request is received from the device.
**/
RETURN_STATUS
EFIAPI
SpdmProcessRequest (
  IN     VOID                    *Context,
     OUT UINT32                  **SessionId,
     OUT BOOLEAN                 *IsAppMessage,
  IN     UINTN                   RequestSize,
  IN     VOID                    *Request
  )
{
  SPDM_DEVICE_CONTEXT       *SpdmContext;
  RETURN_STATUS             Status;
  SPDM_SESSION_INFO         *SessionInfo;
  UINT32                    *MessageSessionId;

  SpdmContext = Context;

  if (Request == NULL) {
    return RETURN_INVALID_PARAMETER;
  }
  if (RequestSize == 0) {
    return RETURN_INVALID_PARAMETER;
  }

  DEBUG((DEBUG_INFO, "SpdmReceiveRequest[.] ...\n"));

  MessageSessionId = NULL;
  SpdmContext->LastSpdmRequestSessionIdValid = FALSE;
  SpdmContext->LastSpdmRequestSize = sizeof(SpdmContext->LastSpdmRequest);
  Status = SpdmContext->TransportDecodeMessage (SpdmContext, &MessageSessionId, IsAppMessage, TRUE, RequestSize, Request, &SpdmContext->LastSpdmRequestSize, SpdmContext->LastSpdmRequest);
  if (RETURN_ERROR(Status)) {
    DEBUG((DEBUG_INFO, "TransportDecodeMessage : %p\n", Status));
    return Status;
  }

  *SessionId = MessageSessionId;

  if (MessageSessionId != NULL) {
    SessionInfo = SpdmGetSessionInfoViaSessionId (SpdmContext, *MessageSessionId);
    if (SessionInfo == NULL) {
      return RETURN_UNSUPPORTED;
    }
    SpdmContext->LastSpdmRequestSessionId = *MessageSessionId;
    SpdmContext->LastSpdmRequestSessionIdValid = TRUE;
  } 

  DEBUG((DEBUG_INFO, "SpdmReceiveRequest[%x] (0x%x): \n", (MessageSessionId != NULL) ? *MessageSessionId : 0, SpdmContext->LastSpdmRequestSize));
  InternalDumpHex ((UINT8 *)SpdmContext->LastSpdmRequest, SpdmContext->LastSpdmRequestSize);

  return RETURN_SUCCESS;
}

/**
  Notify the session state to a session APP.

  @param  SpdmContext                  A pointer to the SPDM context.
  @param  SessionId                    The SessionId of a session.
  @param  SessionState                 The state of a session.
**/
VOID
SpdmTriggerSessionStateCallback (
  IN     SPDM_DEVICE_CONTEXT  *SpdmContext,
  IN     UINT32               SessionId,
  IN     SPDM_SESSION_STATE   SessionState
  )
{
  UINTN                    Index;

  for (Index = 0; Index < MAX_SPDM_SESSION_STATE_CALLBACK_NUM; Index++) {
    if (SpdmContext->SpdmSessionStateCallback[Index] != 0) {
      ((SPDM_SESSION_STATE_CALLBACK)SpdmContext->SpdmSessionStateCallback[Index]) (SpdmContext, SessionId, SessionState);
    }
  }
}

/**
  Build a SPDM response to a device.
  
  @param  SpdmContext                  The SPDM context for the device.
  @param  SessionId                    Indicate if the response is a secured message.
                                       If SessionId is NULL, it is a normal message.
                                       If SessionId is NOT NULL, it is a secured message.
  @param  IsAppMessage                 Indicates if it is an APP message or SPDM message.
  @param  ResponseSize                 Size in bytes of the response data buffer.
  @param  Response                     A pointer to a destination buffer to store the response.
                                       The caller is responsible for having
                                       either implicit or explicit ownership of the buffer.

  @retval RETURN_SUCCESS               The SPDM response is sent successfully.
  @retval RETURN_DEVICE_ERROR          A device error occurs when the SPDM response is sent to the device.
**/
RETURN_STATUS
EFIAPI
SpdmBuildResponse (
  IN     VOID                    *Context,
  IN     UINT32                  *SessionId,
  IN     BOOLEAN                 IsAppMessage,
  IN OUT UINTN                   *ResponseSize,
     OUT VOID                    *Response
  )
{
  SPDM_DEVICE_CONTEXT               *SpdmContext;
  UINT8                             MyResponse[MAX_SPDM_MESSAGE_BUFFER_SIZE];
  UINTN                             MyResponseSize;
  RETURN_STATUS                     Status;
  SPDM_GET_SPDM_RESPONSE_FUNC       GetResponseFunc;
  SPDM_SESSION_INFO                 *SessionInfo;
  SPDM_MESSAGE_HEADER               *SpdmRequest;
  SPDM_MESSAGE_HEADER               *SpdmResponse;

  SpdmContext = Context;

  if (SessionId != NULL) {
    SessionInfo = SpdmGetSessionInfoViaSessionId (SpdmContext, *SessionId);
    if (SessionInfo == NULL) {
      ASSERT (FALSE);
      return RETURN_UNSUPPORTED;
    }
  }

  if (Response == NULL) {
    return RETURN_INVALID_PARAMETER;
  }
  if (ResponseSize == NULL) {
    return RETURN_INVALID_PARAMETER;
  }
  if (*ResponseSize == 0) {
    return RETURN_INVALID_PARAMETER;
  }

  DEBUG((DEBUG_INFO, "SpdmSendResponse[%x] ...\n", (SessionId != NULL) ? *SessionId : 0));

  SpdmRequest = (VOID *)SpdmContext->LastSpdmRequest;
  if (SpdmContext->LastSpdmRequestSize == 0) {
    return RETURN_NOT_READY;
  }

  MyResponseSize = sizeof(MyResponse);
  ZeroMem (MyResponse, sizeof(MyResponse));
  GetResponseFunc = NULL;
  if (!IsAppMessage) {
    GetResponseFunc = SpdmGetResponseFuncViaLastRequest (SpdmContext);
    if (GetResponseFunc != NULL) {
      Status = GetResponseFunc (SpdmContext, SpdmContext->LastSpdmRequestSize, SpdmContext->LastSpdmRequest, &MyResponseSize, MyResponse);
    }
  }
  if (IsAppMessage || (GetResponseFunc == NULL)) {
    if (SpdmContext->GetResponseFunc != 0) {
      Status = ((SPDM_GET_RESPONSE_FUNC)SpdmContext->GetResponseFunc) (SpdmContext, SessionId, IsAppMessage, SpdmContext->LastSpdmRequestSize, SpdmContext->LastSpdmRequest, &MyResponseSize, MyResponse);
    } else {
      Status = RETURN_NOT_FOUND;
    }
  }
  if (Status != RETURN_SUCCESS) {
    SpdmGenerateErrorResponse (SpdmContext, SPDM_ERROR_CODE_UNSUPPORTED_REQUEST, SpdmRequest->RequestResponseCode, &MyResponseSize, MyResponse);
  }

  DEBUG((DEBUG_INFO, "SpdmSendResponse[%x] (0x%x): \n", (SessionId != NULL) ? *SessionId : 0, MyResponseSize));
  InternalDumpHex (MyResponse, MyResponseSize);

  Status = SpdmContext->TransportEncodeMessage (SpdmContext, SessionId, IsAppMessage, FALSE, MyResponseSize, MyResponse, ResponseSize, Response);
  if (RETURN_ERROR(Status)) {
    DEBUG((DEBUG_INFO, "TransportEncodeMessage : %p\n", Status));
    return Status;
  }

  SpdmResponse = (VOID *)MyResponse;
  if (SessionId != NULL) {
    switch (SpdmResponse->RequestResponseCode) {
    case SPDM_FINISH_RSP:
      if ((SpdmContext->ConnectionInfo.Capability.Flags & SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_HANDSHAKE_IN_THE_CLEAR_CAP) == 0) {
        SpdmSecuredMessageSetSessionState (SessionInfo->SecuredMessageContext, SpdmSessionStateEstablished);
        SpdmTriggerSessionStateCallback (Context, SessionInfo->SessionId, SpdmSessionStateEstablished);
      }
      break;
    case SPDM_PSK_FINISH_RSP:
      SpdmSecuredMessageSetSessionState (SessionInfo->SecuredMessageContext, SpdmSessionStateEstablished);
      SpdmTriggerSessionStateCallback (Context, SessionInfo->SessionId, SpdmSessionStateEstablished);
      break;
    case SPDM_END_SESSION_ACK:
      SpdmSecuredMessageSetSessionState (SessionInfo->SecuredMessageContext, SpdmSessionStateNotStarted);
      SpdmTriggerSessionStateCallback (Context, SessionInfo->SessionId, SpdmSessionStateNotStarted);
      SpdmFreeSessionId(SpdmContext, *SessionId);
      break;
    }
  } else {
    switch (SpdmResponse->RequestResponseCode) {
    case SPDM_FINISH_RSP:
      if ((SpdmContext->ConnectionInfo.Capability.Flags & SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_HANDSHAKE_IN_THE_CLEAR_CAP) != 0) {
        SessionInfo = SpdmGetSessionInfoViaSessionId (SpdmContext, SpdmContext->LatestSessionId);
        if (SessionInfo == NULL) {
          ASSERT(FALSE);
          return RETURN_SUCCESS;
        }
        SpdmSecuredMessageSetSessionState (SessionInfo->SecuredMessageContext, SpdmSessionStateEstablished);
        SpdmTriggerSessionStateCallback (Context, SessionInfo->SessionId, SpdmSessionStateEstablished);
      }
      break;
    }
  }
  
  return RETURN_SUCCESS;
}

/**
  Register an SPDM or APP message process function.

  If the default message process function cannot handle the message,
  this function will be invoked.

  @param  SpdmContext                  A pointer to the SPDM context.
  @param  GetResponseFunc              The function to process the encapsuled message.
**/
VOID
EFIAPI
SpdmRegisterGetResponseFunc (
  IN  VOID                    *Context,
  IN  SPDM_GET_RESPONSE_FUNC  GetResponseFunc
  )
{
  SPDM_DEVICE_CONTEXT      *SpdmContext;

  SpdmContext = Context;
  SpdmContext->GetResponseFunc = (UINTN)GetResponseFunc;

  return ;
}

/**
  Register an SPDM state callback function.

  This function can be called multiple times to let different session APPs register its own callback.

  @param  SpdmContext                  A pointer to the SPDM context.
  @param  SpdmSessionStateCallback     The function to be called in SPDM session state change.

  @retval RETURN_SUCCESS          The callback is registered.
  @retval RETURN_ALREADY_STARTED  No enough memory to register the callback.
**/
RETURN_STATUS
EFIAPI
SpdmRegisterSessionStateCallback (
  IN  VOID                         *Context,
  IN  SPDM_SESSION_STATE_CALLBACK  SpdmSessionStateCallback
  )
{
  SPDM_DEVICE_CONTEXT      *SpdmContext;
  UINTN                    Index;

  SpdmContext = Context;
  for (Index = 0; Index < MAX_SPDM_SESSION_STATE_CALLBACK_NUM; Index++) {
    if (SpdmContext->SpdmSessionStateCallback[Index] == 0) {
      SpdmContext->SpdmSessionStateCallback[Index] = (UINTN)SpdmSessionStateCallback;
      return RETURN_SUCCESS;
    }
  }
  ASSERT(FALSE);

  return RETURN_ALREADY_STARTED;
}
