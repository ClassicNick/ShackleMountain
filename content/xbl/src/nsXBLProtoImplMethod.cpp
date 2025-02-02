/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   David Hyatt <hyatt@netscape.com> (Original Author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsIAtom.h"
#include "nsString.h"
#include "jsapi.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIScriptGlobalObject.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsReadableUtils.h"
#include "nsXBLProtoImplMethod.h"
#include "nsIScriptContext.h"
#include "nsContentUtils.h"
#include "nsIScriptSecurityManager.h"

MOZ_DECL_CTOR_COUNTER(nsXBLProtoImplMethod)

nsXBLProtoImplMethod::nsXBLProtoImplMethod(const PRUnichar* aName) :
  nsXBLProtoImplMember(aName), 
  mUncompiledMethod(nsnull)
#ifdef DEBUG
  , mIsCompiled(PR_FALSE)
#endif
{
  MOZ_COUNT_CTOR(nsXBLProtoImplMethod);
}

nsXBLProtoImplMethod::~nsXBLProtoImplMethod()
{
  MOZ_COUNT_DTOR(nsXBLProtoImplMethod);
}

void
nsXBLProtoImplMethod::Destroy(PRBool aIsCompiled)
{
  NS_PRECONDITION(aIsCompiled == mIsCompiled,
                  "Incorrect aIsCompiled in nsXBLProtoImplMethod::Destroy");
  if (aIsCompiled) {
    if (mJSMethodObject)
      nsContentUtils::RemoveJSGCRoot(&mJSMethodObject);
    mJSMethodObject = nsnull;
  }
  else {
    delete mUncompiledMethod;
    mUncompiledMethod = nsnull;
  }
}

void 
nsXBLProtoImplMethod::AppendBodyText(const nsAString& aText)
{
  NS_PRECONDITION(!mIsCompiled,
                  "Must not be compiled when accessing uncompiled method");
  if (!mUncompiledMethod) {
    mUncompiledMethod = new nsXBLUncompiledMethod();
    if (!mUncompiledMethod)
      return;
  }

  mUncompiledMethod->AppendBodyText(aText);
}

void 
nsXBLProtoImplMethod::AddParameter(const nsAString& aText)
{
  NS_PRECONDITION(!mIsCompiled,
                  "Must not be compiled when accessing uncompiled method");
  if (!mUncompiledMethod) {
    mUncompiledMethod = new nsXBLUncompiledMethod();
    if (!mUncompiledMethod)
      return;
  }

  mUncompiledMethod->AddParameter(aText);
}

void
nsXBLProtoImplMethod::SetLineNumber(PRUint32 aLineNumber)
{
  NS_PRECONDITION(!mIsCompiled,
                  "Must not be compiled when accessing uncompiled method");
  if (!mUncompiledMethod) {
    mUncompiledMethod = new nsXBLUncompiledMethod();
    if (!mUncompiledMethod)
      return;
  }

  mUncompiledMethod->SetLineNumber(aLineNumber);
}

nsresult
nsXBLProtoImplMethod::InstallMember(nsIScriptContext* aContext,
                                    nsIContent* aBoundElement, 
                                    void* aScriptObject,
                                    void* aTargetClassObject,
                                    const nsCString& aClassStr)
{
  NS_PRECONDITION(mIsCompiled,
                  "Should not be installing an uncompiled method");
  JSContext* cx = (JSContext*) aContext->GetNativeContext();

  nsIDocument *ownerDoc = aBoundElement->GetOwnerDoc();
  nsIScriptGlobalObject *sgo;

  if (!ownerDoc || !(sgo = ownerDoc->GetScriptGlobalObject())) {
    NS_ERROR("Can't find global object for bound content!");
 
    return NS_ERROR_UNEXPECTED;
  }

  JSObject * scriptObject = (JSObject *) aScriptObject;
  NS_ASSERTION(scriptObject, "uh-oh, script Object should NOT be null or bad things will happen");
  if (!scriptObject)
    return NS_ERROR_FAILURE;

  JSObject * targetClassObject = (JSObject *) aTargetClassObject;
  JSObject * globalObject = sgo->GetGlobalJSObject();

  // now we want to reevaluate our property using aContext and the script object for this window...
  if (mJSMethodObject && targetClassObject) {
    nsDependentString name(mName);
    JSObject * method = ::JS_CloneFunctionObject(cx, mJSMethodObject, globalObject);
    if (!method) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    nsresult rv;
    nsAutoGCRoot root(&method, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (!::JS_DefineUCProperty(cx, targetClassObject,
                               NS_REINTERPRET_CAST(const jschar*, mName), 
                               name.Length(), OBJECT_TO_JSVAL(method),
                               NULL, NULL, JSPROP_ENUMERATE)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  return NS_OK;
}

nsresult 
nsXBLProtoImplMethod::CompileMember(nsIScriptContext* aContext, const nsCString& aClassStr,
                                    void* aClassObject)
{
  NS_PRECONDITION(!mIsCompiled,
                  "Trying to compile an already-compiled method");
  NS_PRECONDITION(aClassObject,
                  "Must have class object to compile");

#ifdef DEBUG
  // We have some "ok" early returns after which we consider ourselves compiled
  mIsCompiled = PR_TRUE;
#endif
  
  // No parameters or body was supplied, so don't install method.
  if (!mUncompiledMethod)
    return NS_OK;

  // Don't install method if no name or body was supplied.
  if (!(mName && mUncompiledMethod->mBodyText.GetText())) {
    delete mUncompiledMethod;
    mUncompiledMethod = nsnull;
    return NS_OK;
  }

  nsDependentString body(mUncompiledMethod->mBodyText.GetText());
  if (body.IsEmpty()) {
    delete mUncompiledMethod;
    mUncompiledMethod = nsnull;
    return NS_OK;
  }

#ifdef DEBUG
  // OK, now we have some error early returns that mean we're not
  // really compiled...
  mIsCompiled = PR_FALSE;
#endif

  // We have a method.
  // Allocate an array for our arguments.
  PRInt32 paramCount = mUncompiledMethod->GetParameterCount();
  char** args = nsnull;
  if (paramCount > 0) {
    args = new char*[paramCount];
    if (!args)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  // Add our parameters to our args array.
  PRInt32 argPos = 0; 
  for (nsXBLParameter* curr = mUncompiledMethod->mParameters; 
       curr; 
       curr = curr->mNext) {
    args[argPos] = curr->mName;
    argPos++;
  }

  // Now that we have a body and args, compile the function
  // and then define it.
  NS_ConvertUCS2toUTF8 cname(mName);
  nsCAutoString functionUri(aClassStr);
  PRInt32 hash = functionUri.RFindChar('#');
  if (hash != kNotFound) {
    functionUri.Truncate(hash);
  }

  JSObject* methodObject = nsnull;
  nsresult rv = aContext->CompileFunction(aClassObject,
                                          cname,
                                          paramCount,
                                          (const char**)args,
                                          body, 
                                          functionUri.get(),
                                          mUncompiledMethod->mBodyText.GetLineNumber(),
                                          PR_TRUE,
                                          (void **) &methodObject);

  // Destroy our uncompiled method and delete our arg list.
  delete mUncompiledMethod;
  delete [] args;
  if (NS_FAILED(rv)) {
    mUncompiledMethod = nsnull;
    return rv;
  }

  mJSMethodObject = methodObject;

  if (methodObject) {
    // Root the compiled prototype script object.
    rv = nsContentUtils::AddJSGCRoot(&mJSMethodObject,
                                     "nsXBLProtoImplMethod::mJSMethodObject");
    if (NS_FAILED(rv)) {
      mJSMethodObject = nsnull;
    }
  }
  
#ifdef DEBUG
  mIsCompiled = NS_SUCCEEDED(rv);
#endif
  return rv;
}

nsresult
nsXBLProtoImplAnonymousMethod::Execute(nsIContent* aBoundElement)
{
  NS_PRECONDITION(mIsCompiled, "Can't execute uncompiled method");
  
  if (!mJSMethodObject) {
    // Nothing to do here
    return NS_OK;
  }

  // Get the script context the same way
  // nsXBLProtoImpl::InstallImplementation does.
  nsIDocument* document = aBoundElement->GetOwnerDoc();
  if (!document) {
    return NS_OK;
  }

  nsIScriptGlobalObject* global = document->GetScriptGlobalObject();
  if (!global) {
    return NS_OK;
  }

  nsCOMPtr<nsIScriptContext> context = global->GetContext();
  if (!context) {
    return NS_OK;
  }
  
  JSContext* cx = (JSContext*) context->GetNativeContext();

  JSObject* globalObject = global->GetGlobalJSObject();

  nsCOMPtr<nsIXPConnectJSObjectHolder> wrapper;
  nsresult rv =
    nsContentUtils::XPConnect()->WrapNative(cx, globalObject,
                                            aBoundElement,
                                            NS_GET_IID(nsISupports),
                                            getter_AddRefs(wrapper));
  NS_ENSURE_SUCCESS(rv, rv);

  JSObject* thisObject;
  rv = wrapper->GetJSObject(&thisObject);
  NS_ENSURE_SUCCESS(rv, rv);

  // Clone the function object, using thisObject as the parent so "this" is in
  // the scope chain of the resulting function (for backwards compat to the
  // days when this was an event handler).
  JSObject* method = ::JS_CloneFunctionObject(cx, mJSMethodObject,
                                              thisObject);
  if (!method) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // Now call the method

  // Use nsCxPusher to make sure we call ScriptEvaluated when we're done.
  nsCxPusher pusher;
  NS_ENSURE_STATE(pusher.Push(aBoundElement));


  // Check whether it's OK to call the method.
  rv = nsContentUtils::GetSecurityManager()->CheckFunctionAccess(cx, method, thisObject);

  JSBool ok = JS_TRUE;
  if (NS_SUCCEEDED(rv)) {
    jsval retval;
    ok = ::JS_CallFunctionValue(cx, thisObject, OBJECT_TO_JSVAL(method),
                                0 /* argc */, nsnull /* argv */, &retval);
  }

  if (!ok) {
    // Tell XPConnect about any pending exceptions. This is needed
    // to avoid dropping JS exceptions in case we got here through
    // nested calls through XPConnect.
    nsContentUtils::NotifyXPCIfExceptionPending(cx);
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}
