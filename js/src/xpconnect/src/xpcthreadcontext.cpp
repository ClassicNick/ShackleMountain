/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sw=4 et tw=80:
 *
 * ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   John Bandhauer <jband@netscape.com> (original author)
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

/* Implement global service to track stack of JSContext per thread. */

#include "xpcprivate.h"

/***************************************************************************/

XPCJSContextStack::XPCJSContextStack()
    : mStack(nsnull),
      mStackBufferSize(0),
      mStackSize(0),
      mSafeJSContext(nsnull),
      mOwnSafeJSContext(nsnull)
{
    // empty...
}

XPCJSContextStack::~XPCJSContextStack()
{
    if(mStack)
    {
        for(PRUint32 i = 0; i < mStackSize; ++i)
            mStack[i].~JSContextAndFrame();

        PR_Free(mStack);
    }
                 
    if(mOwnSafeJSContext)
    {
        JS_SetContextThread(mOwnSafeJSContext);
        JS_DestroyContext(mOwnSafeJSContext);
        mOwnSafeJSContext = nsnull;
        SyncJSContexts();
    }
}

void
XPCJSContextStack::SyncJSContexts()
{
    nsXPConnect* xpc = nsXPConnect::GetXPConnect();
    if(xpc)
        xpc->SyncJSContexts();
}

nsresult
XPCJSContextStack::EnsureStackSize(PRUint32 aNewSize)
{
    if(aNewSize <= mStackBufferSize)
    {
        return NS_OK;
    }

    PRUint32 newSize = aNewSize * sizeof(JSContextAndFrame);

    if(!mStack)
    {
        mStack = NS_REINTERPRET_CAST(JSContextAndFrame*, PR_Malloc(newSize));
        NS_ENSURE_TRUE(mStack, NS_ERROR_UNEXPECTED);
    }
    else
    {
        JSContextAndFrame* newStack =
            NS_REINTERPRET_CAST(JSContextAndFrame*,
                                PR_Realloc(mStack, newSize));
        NS_ENSURE_TRUE(newStack, NS_ERROR_OUT_OF_MEMORY);
        mStack = newStack;
    }

    mStackBufferSize = aNewSize;
    return NS_OK;
}

/* readonly attribute PRInt32 count; */
NS_IMETHODIMP
XPCJSContextStack::GetCount(PRInt32 *aCount)
{
    *aCount = mStackSize;
    return NS_OK;
}

/* JSContext peek (); */
NS_IMETHODIMP
XPCJSContextStack::Peek(JSContext * *_retval)
{
    *_retval = mStackSize == 0 ? nsnull : mStack[mStackSize - 1].cx;
    return NS_OK;
}

/* JSContext pop (); */
NS_IMETHODIMP
XPCJSContextStack::Pop(JSContext * *_retval)
{
    NS_ASSERTION(mStackSize != 0, "ThreadJSContextStack underflow");

    PRUint32 idx = mStackSize - 1; // The thing we're popping
    NS_ASSERTION(!mStack[idx].frame,
                 "Shouldn't have a pending frame to restore on the context "
                 "we're popping!");

    if(_retval)
        *_retval = mStack[idx].cx;

    mStack[idx].~JSContextAndFrame();
    --mStackSize;
    if(idx > 0)
    {
        --idx; // Advance to new top of the stack
        JSContextAndFrame & e = mStack[idx];
        NS_ASSERTION(!e.frame || e.cx, "Shouldn't have frame without a cx!");
        if(e.cx && e.frame)
        {
            JS_RestoreFrameChain(e.cx, e.frame);
            e.frame = nsnull;
        }
    }

    if(mStackSize < mStackBufferSize / 2)
    {
        mStackBufferSize = mStackSize;
        mStack = NS_REINTERPRET_CAST(JSContextAndFrame*,
                                     PR_Realloc(mStack,
                                                mStackBufferSize *
                                                  sizeof(JSContextAndFrame)));
    }
    
    return NS_OK;
}

/* void push (in JSContext cx); */
NS_IMETHODIMP
XPCJSContextStack::Push(JSContext * cx)
{
    nsresult rv = EnsureStackSize(mStackSize + 1);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ASSERTION(mStackBufferSize > mStackSize, "How did that happen?");
    new (mStack + mStackSize) JSContextAndFrame(cx);
    ++mStackSize;
    if(mStackSize > 1)
    {
        JSContextAndFrame & e = mStack[mStackSize - 2];
        if(e.cx && e.cx != cx)
            e.frame = JS_SaveFrameChain(e.cx);
    }
    return NS_OK;
}

#ifdef DEBUG
JSBool 
XPCJSContextStack::DEBUG_StackHasJSContext(JSContext*  aJSContext)
{
    for(PRUint32 i = 0; i < mStackSize; i++)
        if(aJSContext == mStack[i].cx)
            return JS_TRUE;
    return JS_FALSE;
}
#endif

JS_STATIC_DLL_CALLBACK(JSBool)
SafeGlobalResolve(JSContext *cx, JSObject *obj, jsval id)
{
    JSBool resolved;
    return JS_ResolveStandardClass(cx, obj, id, &resolved);
}

static JSClass global_class = {
    "global_for_XPCJSContextStack_SafeJSContext", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, SafeGlobalResolve, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

/* attribute JSContext safeJSContext; */
NS_IMETHODIMP
XPCJSContextStack::GetSafeJSContext(JSContext * *aSafeJSContext)
{
    if(!mSafeJSContext)
    {
        JSRuntime *rt;
        XPCJSRuntime* xpcrt;

        nsXPConnect* xpc = nsXPConnect::GetXPConnect();
        nsCOMPtr<nsIXPConnect> xpcholder(NS_STATIC_CAST(nsIXPConnect*, xpc));

        if(xpc && (xpcrt = xpc->GetRuntime()) && (rt = xpcrt->GetJSRuntime()))
        {
            mSafeJSContext = JS_NewContext(rt, 8192);
            if(mSafeJSContext)
            {
                // scoped JS Request
                AutoJSRequestWithNoCallContext req(mSafeJSContext);
                JSObject *glob;
                glob = JS_NewObject(mSafeJSContext, &global_class, NULL, NULL);
                if(!glob || NS_FAILED(xpc->InitClasses(mSafeJSContext, glob)))
                {
                    // Explicitly end the request since we are about to kill
                    // the JSContext that 'req' will try to use when it
                    // goes out of scope.
                    req.EndRequest();
                    JS_DestroyContext(mSafeJSContext);
                    mSafeJSContext = nsnull;
                }
                // Save it off so we can destroy it later, even if
                // mSafeJSContext has been set to another context
                // via SetSafeJSContext.  If we don't get here,
                // then mSafeJSContext must have been set via
                // SetSafeJSContext, and we're not responsible for
                // destroying the passed-in context.
                mOwnSafeJSContext = mSafeJSContext;
            }
        }
    }

    *aSafeJSContext = mSafeJSContext;
    return mSafeJSContext ? NS_OK : NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
XPCJSContextStack::SetSafeJSContext(JSContext * aSafeJSContext)
{
    if(mOwnSafeJSContext &&
       mOwnSafeJSContext == mSafeJSContext &&
       mOwnSafeJSContext != aSafeJSContext)
    {
        JS_DestroyContext(mOwnSafeJSContext);
        mOwnSafeJSContext = nsnull;
        SyncJSContexts();
    }

    mSafeJSContext = aSafeJSContext;
    return NS_OK;
}

/***************************************************************************/

/*
 * nsXPCThreadJSContextStackImpl holds state that we don't want to lose!
 *
 * The plan is that once created nsXPCThreadJSContextStackImpl never goes
 * away until FreeSingleton is called. We do an intentional extra addref at
 * construction to keep it around even if no one is using it.
 */

NS_IMPL_THREADSAFE_ISUPPORTS3(nsXPCThreadJSContextStackImpl,
                              nsIThreadJSContextStack,
                              nsIJSContextStack,
                              nsISupportsWeakReference)

nsXPCThreadJSContextStackImpl*
nsXPCThreadJSContextStackImpl::gXPCThreadJSContextStack = nsnull;

nsXPCThreadJSContextStackImpl::nsXPCThreadJSContextStackImpl()
{
}

nsXPCThreadJSContextStackImpl::~nsXPCThreadJSContextStackImpl()
{
    gXPCThreadJSContextStack = nsnull;
}

//static
nsXPCThreadJSContextStackImpl*
nsXPCThreadJSContextStackImpl::GetSingleton()
{
    if(!gXPCThreadJSContextStack)
    {
        gXPCThreadJSContextStack = new nsXPCThreadJSContextStackImpl();
        // hold an extra reference to lock it down
        NS_IF_ADDREF(gXPCThreadJSContextStack);
    }
    NS_IF_ADDREF(gXPCThreadJSContextStack);

    return gXPCThreadJSContextStack;
}

void
nsXPCThreadJSContextStackImpl::FreeSingleton()
{
    nsXPCThreadJSContextStackImpl* tcs = gXPCThreadJSContextStack;
    if(tcs)
    {
        nsrefcnt cnt;
        NS_RELEASE2(tcs, cnt);
#ifdef XPC_DUMP_AT_SHUTDOWN
        if(0 != cnt)
            printf("*** dangling reference to nsXPCThreadJSContextStackImpl: refcnt=%d\n", cnt);
#endif
    }
}

/* readonly attribute PRInt32 Count; */
NS_IMETHODIMP
nsXPCThreadJSContextStackImpl::GetCount(PRInt32 *aCount)
{
    if(!aCount)
        return NS_ERROR_NULL_POINTER;

    XPCJSContextStack* myStack = GetStackForCurrentThread();

    if(!myStack)
    {
        *aCount = 0;
        return NS_ERROR_FAILURE;
    }

    return myStack->GetCount(aCount);
}

/* JSContext Peek (); */
NS_IMETHODIMP
nsXPCThreadJSContextStackImpl::Peek(JSContext * *_retval)
{
    if(!_retval)
        return NS_ERROR_NULL_POINTER;

    XPCJSContextStack* myStack = GetStackForCurrentThread();

    if(!myStack)
    {
        *_retval = nsnull;
        return NS_ERROR_FAILURE;
    }

    return myStack->Peek(_retval);
}

/* JSContext Pop (); */
NS_IMETHODIMP
nsXPCThreadJSContextStackImpl::Pop(JSContext * *_retval)
{
    XPCJSContextStack* myStack = GetStackForCurrentThread();

    if(!myStack)
    {
        if(_retval)
            *_retval = nsnull;
        return NS_ERROR_FAILURE;
    }

    return myStack->Pop(_retval);
}

/* void Push (in JSContext cx); */
NS_IMETHODIMP
nsXPCThreadJSContextStackImpl::Push(JSContext * cx)
{
    XPCJSContextStack* myStack = GetStackForCurrentThread();

    if(!myStack)
        return NS_ERROR_FAILURE;

    return myStack->Push(cx);
}

/* readonly attribute JSContext SafeJSContext; */
NS_IMETHODIMP
nsXPCThreadJSContextStackImpl::GetSafeJSContext(JSContext * *aSafeJSContext)
{
    NS_ASSERTION(aSafeJSContext, "loser!");

    XPCJSContextStack* myStack = GetStackForCurrentThread();

    if(!myStack)
    {
        *aSafeJSContext = nsnull;
        return NS_ERROR_FAILURE;
    }

    return myStack->GetSafeJSContext(aSafeJSContext);
}


NS_IMETHODIMP
nsXPCThreadJSContextStackImpl::SetSafeJSContext(JSContext * aSafeJSContext)
{
    XPCJSContextStack* myStack = GetStackForCurrentThread();

    if(!myStack)
        return NS_ERROR_FAILURE;

    return myStack->SetSafeJSContext(aSafeJSContext);
}

/***************************************************************************/

PRUintn           XPCPerThreadData::gTLSIndex = BAD_TLS_INDEX;
PRLock*           XPCPerThreadData::gLock     = nsnull;
XPCPerThreadData* XPCPerThreadData::gThreads  = nsnull;

XPCPerThreadData::XPCPerThreadData()
    :   mJSContextStack(new XPCJSContextStack()),
        mNextThread(nsnull),
        mCallContext(nsnull),
        mResolveName(0),
        mResolvingWrapper(nsnull),
        mMostRecentJSContext(nsnull),
        mMostRecentXPCContext(nsnull),
        mExceptionManager(nsnull),
        mException(nsnull),
        mExceptionManagerNotAvailable(JS_FALSE),
        mAutoRoots(nsnull)
#ifdef XPC_CHECK_WRAPPER_THREADSAFETY
      , mWrappedNativeThreadsafetyReportDepth(0)
#endif
{
    if(gLock)
    {
        nsAutoLock lock(gLock);
        mNextThread = gThreads;
        gThreads = this;
    }
}

void
XPCPerThreadData::Cleanup()
{
    while(mAutoRoots)
        mAutoRoots->Unlink();
    NS_IF_RELEASE(mExceptionManager);
    NS_IF_RELEASE(mException);
    delete mJSContextStack;
    mJSContextStack = nsnull;

    if(mCallContext)
        mCallContext->SystemIsBeingShutDown();
}

XPCPerThreadData::~XPCPerThreadData()
{
    Cleanup();

    // Unlink 'this' from the list of threads.
    if(gLock)
    {
        nsAutoLock lock(gLock);
        if(gThreads == this)
            gThreads = mNextThread;
        else
        {
            XPCPerThreadData* cur = gThreads;
            while(cur)
            {
                if(cur->mNextThread == this)
                {
                    cur->mNextThread = mNextThread;
                    break;
                }
                cur = cur->mNextThread;
            }
        }
    }

    if(gLock && !gThreads)
    {
        PR_DestroyLock(gLock);
        gLock = nsnull;
    }
}

PR_STATIC_CALLBACK(void)
xpc_ThreadDataDtorCB(void* ptr)
{
    XPCPerThreadData* data = (XPCPerThreadData*) ptr;
    if(data)
        delete data;
}

void XPCPerThreadData::MarkAutoRootsBeforeJSFinalize(JSContext* cx)
{
#ifdef XPC_TRACK_AUTOMARKINGPTR_STATS
    {
        static int maxLength = 0;
        int length = 0;
        for(AutoMarkingPtr* p = mAutoRoots; p; p = p->GetNext())
            length++;
        if(length > maxLength)
            maxLength = length;
        printf("XPC gc on thread %x with %d AutoMarkingPtrs (%d max so far)\n",
               this, length, maxLength);
    }
#endif

    if(mAutoRoots)
        mAutoRoots->MarkBeforeJSFinalize(cx);
}

void XPCPerThreadData::MarkAutoRootsAfterJSFinalize()
{
    if(mAutoRoots)
        mAutoRoots->MarkAfterJSFinalize();
}

// static
XPCPerThreadData*
XPCPerThreadData::GetData()
{
    XPCPerThreadData* data;

    if(!gLock)
    {
        gLock = PR_NewLock();
        if(!gLock)
            return nsnull;
    }

    if(gTLSIndex == BAD_TLS_INDEX)
    {
        nsAutoLock lock(gLock);
        // check again now that we have the lock...
        if(gTLSIndex == BAD_TLS_INDEX)
        {
            if(PR_FAILURE ==
               PR_NewThreadPrivateIndex(&gTLSIndex, xpc_ThreadDataDtorCB))
            {
                NS_ASSERTION(0, "PR_NewThreadPrivateIndex failed!");
                gTLSIndex = BAD_TLS_INDEX;
                return nsnull;
            }
        }
    }

    data = (XPCPerThreadData*) PR_GetThreadPrivate(gTLSIndex);
    if(!data)
    {
        data = new XPCPerThreadData();
        if(!data || !data->IsValid())
        {
            NS_ASSERTION(0, "new XPCPerThreadData() failed!");
            if(data)
                delete data;
            return nsnull;
        }
        if(PR_FAILURE == PR_SetThreadPrivate(gTLSIndex, data))
        {
            NS_ASSERTION(0, "PR_SetThreadPrivate failed!");
            delete data;
            return nsnull;
        }
    }
    return data;
}

// static
void
XPCPerThreadData::CleanupAllThreads()
{
    // I've questioned the sense of cleaning up other threads' data from the
    // start. But I got talked into it. Now I see that we *can't* do all the
    // cleaup while holding this lock. So, we are going to go to the trouble
    // to copy out the data that needs to be cleaned up *outside* of
    // the lock. Yuk!

    XPCJSContextStack** stacks = nsnull;
    int count = 0;
    int i;

    if(gLock)
    {
        nsAutoLock lock(gLock);

        for(XPCPerThreadData* cur = gThreads; cur; cur = cur->mNextThread)
            count++;

        stacks = (XPCJSContextStack**) new XPCJSContextStack*[count] ;
        if(stacks)
        {
            i = 0;
            for(XPCPerThreadData* cur = gThreads; cur; cur = cur->mNextThread)
            {
                stacks[i++] = cur->mJSContextStack;
                cur->mJSContextStack = nsnull;
                cur->Cleanup();
            }
        }
    }

    if(stacks)
    {
        for(i = 0; i < count; i++)
            delete stacks[i];
        delete [] stacks;
    }

    if(gTLSIndex != BAD_TLS_INDEX)
        PR_SetThreadPrivate(gTLSIndex, nsnull);
}

// static
XPCPerThreadData*
XPCPerThreadData::IterateThreads(XPCPerThreadData** iteratorp)
{
    *iteratorp = (*iteratorp == nsnull) ? gThreads : (*iteratorp)->mNextThread;
    return *iteratorp;
}

NS_IMPL_ISUPPORTS1(nsXPCJSContextStackIterator, nsIJSContextStackIterator)

NS_IMETHODIMP
nsXPCJSContextStackIterator::Reset(nsIJSContextStack *aStack)
{
    // XXX This is pretty ugly.
    nsXPCThreadJSContextStackImpl *impl =
        NS_STATIC_CAST(nsXPCThreadJSContextStackImpl*, aStack);
    XPCJSContextStack *stack = impl->GetStackForCurrentThread();
    if(!stack)
        return NS_ERROR_FAILURE;
    mStack = stack->GetStack();
    mStackSize = stack->GetStackSize();
    if(mStackSize == 0)
        mStack = nsnull;
    else
        mPosition = mStackSize - 1;

    return NS_OK;
}

NS_IMETHODIMP
nsXPCJSContextStackIterator::Done(PRBool *aDone)
{
    *aDone = !mStack;
    return NS_OK;
}

NS_IMETHODIMP
nsXPCJSContextStackIterator::Prev(JSContext **aContext)
{
    if(!mStack)
        return NS_ERROR_NOT_INITIALIZED;

    *aContext = (*mStack)[mPosition].cx;

    if(mPosition == 0)
        mStack = nsnull;
    else
        --mPosition;
    
    return NS_OK;
}

