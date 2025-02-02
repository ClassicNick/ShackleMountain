/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 sw=2 et tw=80: */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Travis Bogard <travis@netscape.com>
 *   Dan Rosen <dr@netscape.com>
 *   Vidur Apparao <vidur@netscape.com>
 *   Johnny Stenback <jst@netscape.com>
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

#ifndef nsGlobalWindow_h___
#define nsGlobalWindow_h___

// Local Includes
// Helper Classes
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsWeakReference.h"
#include "nsHashtable.h"

// Interfaces Needed
#include "nsDOMWindowList.h"
#include "nsIBaseWindow.h"
#include "nsIBrowserDOMWindow.h"
#include "nsIChromeEventHandler.h"
#include "nsIControllers.h"
#include "nsIObserver.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMViewCSS.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOM3EventTarget.h"
#include "nsIDOMNSEventTarget.h"
#include "nsIDOMNavigator.h"
#include "nsIDOMNSLocation.h"
#include "nsIDOMWindowInternal.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDOMJSWindow.h"
#include "nsIDOMChromeWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsITimer.h"
#include "nsIWebBrowserChrome.h"
#include "nsPIDOMWindow.h"
#include "nsIScriptSecurityManager.h"
#include "nsIEventListenerManager.h"
#include "nsIDOMDocument.h"
#include "nsIDOMCrypto.h"
#include "nsIDOMPkcs11.h"
#include "nsIPrincipal.h"
#include "nsPluginArray.h"
#include "nsMimeTypeArray.h"
#include "nsIXPCScriptable.h"
#include "nsPoint.h"
#include "nsSize.h"
#include "mozFlushType.h"
#include "prclist.h"

#define DEFAULT_HOME_PAGE "www.mozilla.org"
#define PREF_BROWSER_STARTUP_HOMEPAGE "browser.startup.homepage"

class nsIDOMBarProp;
class nsIDocument;
class nsIContent;
class nsPresContext;
class nsIDOMEvent;
class nsIScrollableView;

typedef struct nsTimeout nsTimeout;

class nsBarProp;
class nsLocation;
class nsNavigator;
class nsScreen;
class nsHistory;
class nsIDocShellLoadInfo;
class WindowStateHolder;

//*****************************************************************************
// nsGlobalWindow: Global Object for Scripting
//*****************************************************************************
// Beware that all scriptable interfaces implemented by
// nsGlobalWindow will be reachable from JS, if you make this class
// implement new interfaces you better know what you're
// doing. Security wise this is very sensitive code. --
// jst@netscape.com

// nsGlobalWindow inherits PRCList for maintaining a list of all inner
// widows still in memory for any given outer window. This list is
// needed to ensure that mOuterWindow doesn't end up dangling. The
// nature of PRCList means that the window itself is always in the
// list, and an outer window's list will also contain all inner window
// objects that are still in memory (and in reality all inner window
// object's lists also contain its outer and all other inner windows
// belonging to the same outer window, but that's an unimportant
// side effect of inheriting PRCList).

class nsGlobalWindow : public nsPIDOMWindow_MOZILLA_1_8_BRANCH2,
                       public nsIScriptGlobalObject,
                       public nsIDOMJSWindow,
                       public nsIScriptObjectPrincipal,
                       public nsIDOMEventReceiver,
                       public nsIDOM3EventTarget,
                       public nsIDOMNSEventTarget,
                       public nsIDOMViewCSS,
                       public nsSupportsWeakReference,
                       public nsIInterfaceRequestor,
                       public PRCListStr
{
public:
  // public methods
  nsPIDOMWindow* GetPrivateParent();
  // callback for close event
  void ReallyCloseWindow();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIScriptGlobalObject
  virtual void SetContext(nsIScriptContext *aContext);
  virtual nsIScriptContext *GetContext();
  virtual nsresult SetNewDocument(nsIDOMDocument *aDocument,
                                  nsISupports *aState,
                                  PRBool aRemoveEventListeners,
                                  PRBool aClearScopeHint);
  virtual void SetDocShell(nsIDocShell* aDocShell);
  virtual nsIDocShell *GetDocShell();
  virtual void SetOpenerWindow(nsIDOMWindowInternal *aOpener);
  virtual void SetGlobalObjectOwner(nsIScriptGlobalObjectOwner* aOwner);
  virtual nsIScriptGlobalObjectOwner *GetGlobalObjectOwner();
  virtual nsresult HandleDOMEvent(nsPresContext* aPresContext,
                                  nsEvent* aEvent, nsIDOMEvent** aDOMEvent,
                                  PRUint32 aFlags,
                                  nsEventStatus* aEventStatus);
  virtual JSObject *GetGlobalJSObject();
  virtual void OnFinalize(JSObject *aJSObject);
  virtual void SetScriptsEnabled(PRBool aEnabled, PRBool aFireTimeouts);
  virtual nsresult SetNewArguments(PRUint32 aArgc, void* aArgv);

  // nsIScriptObjectPrincipal
  virtual nsIPrincipal* GetPrincipal();

  // nsIDOMWindow
  NS_DECL_NSIDOMWINDOW

  // nsIDOMWindow2
  NS_DECL_NSIDOMWINDOW2

  // nsIDOMWindowInternal
  NS_DECL_NSIDOMWINDOWINTERNAL

  // nsIDOMJSWindow
  NS_DECL_NSIDOMJSWINDOW

  // nsIDOMEventTarget
  NS_DECL_NSIDOMEVENTTARGET

  // nsIDOM3EventTarget
  NS_DECL_NSIDOM3EVENTTARGET

  // nsIDOMNSEventTarget
  NS_DECL_NSIDOMNSEVENTTARGET

  // nsIDOMEventReceiver
  NS_IMETHOD AddEventListenerByIID(nsIDOMEventListener *aListener,
                                   const nsIID& aIID);
  NS_IMETHOD RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                      const nsIID& aIID);
  NS_IMETHOD GetListenerManager(nsIEventListenerManager** aInstancePtrResult);
  NS_IMETHOD HandleEvent(nsIDOMEvent *aEvent);
  NS_IMETHOD GetSystemEventGroup(nsIDOMEventGroup** aGroup);

  // nsPIDOMWindow
  virtual NS_HIDDEN_(nsPIDOMWindow*) GetPrivateRoot();
  virtual NS_HIDDEN_(nsresult) GetObjectProperty(const PRUnichar* aProperty,
                                                 nsISupports** aObject);
  virtual NS_HIDDEN_(nsresult) Activate();
  virtual NS_HIDDEN_(nsresult) Deactivate();
  virtual NS_HIDDEN_(nsIFocusController*) GetRootFocusController();

  virtual NS_HIDDEN_(void) SetOpenerScriptURL(nsIURI* aURI);

  virtual NS_HIDDEN_(PopupControlState) PushPopupControlState(PopupControlState state, PRBool aForce) const;
  virtual NS_HIDDEN_(void) PopPopupControlState(PopupControlState state) const;
  virtual NS_HIDDEN_(PopupControlState) GetPopupControlState() const;
  virtual NS_HIDDEN_(OpenAllowValue) GetOpenAllow(const nsAString &aName);

  virtual NS_HIDDEN_(nsresult) SaveWindowState(nsISupports **aState);
  virtual NS_HIDDEN_(nsresult) RestoreWindowState(nsISupports *aState);

  virtual NS_HIDDEN_(PRBool) WouldReuseInnerWindow(nsIDocument *aNewDocument);

  // nsPIDOMWindow_MOZILLA_1_8_BRANCH2
  virtual NS_HIDDEN_(void) SetOpenerScriptPrincipal(nsIPrincipal* aPrincipal);
  virtual NS_HIDDEN_(nsIPrincipal*) GetOpenerScriptPrincipal();
  
  // nsIDOMViewCSS
  NS_DECL_NSIDOMVIEWCSS

  // nsIDOMAbstractView
  NS_DECL_NSIDOMABSTRACTVIEW

  // nsIInterfaceRequestor
  NS_DECL_NSIINTERFACEREQUESTOR

  // Object Management
  nsGlobalWindow(nsGlobalWindow *aOuterWindow);

  static nsGlobalWindow *FromWrapper(nsIXPConnectWrappedNative *wrapper)
  {
    // Make sure this matches the casts we do in QueryInterface().
    return (nsGlobalWindow *)(nsIScriptGlobalObject *)wrapper->Native();
  }

  nsIScriptContext *GetContextInternal()
  {
    if (mOuterWindow) {
      return GetOuterWindowInternal()->mContext;
    }

    return mContext;
  }

  nsGlobalWindow *GetOuterWindowInternal()
  {
    return NS_STATIC_CAST(nsGlobalWindow *, GetOuterWindow());
  }

  nsGlobalWindow *GetCurrentInnerWindowInternal()
  {
    return NS_STATIC_CAST(nsGlobalWindow *, mInnerWindow);
  }

  nsIDocShell *GetDocShellInternal()
  {
    if (mOuterWindow) {
      return GetOuterWindowInternal()->mDocShell;
    }

    return mDocShell;
  }

  PRBool IsFrozen() const
  {
    return mIsFrozen;
  }

  static void ShutDown();
  static PRBool IsCallerChrome();

  friend class WindowStateHolder;

protected:
  // Object Management
  virtual ~nsGlobalWindow();
  void CleanUp();
  void ClearControllers();

  void FreeInnerObjects(JSContext *cx);

  nsresult SetNewDocument(nsIDOMDocument *aDocument,
                          nsISupports *aState,
                          PRBool aRemoveEventListeners,
                          PRBool aClearScopeHint,
                          PRBool aIsInternalCall);

  // Get the parent, returns null if this is a toplevel window
  nsIDOMWindowInternal *GetParentInternal();

  // popup tracking
  PRBool IsPopupSpamWindow()
  {
    if (IsInnerWindow() && !mOuterWindow) {
      return PR_FALSE;
    }

    return GetOuterWindowInternal()->mIsPopupSpam;
  }

  void SetPopupSpamWindow(PRBool aPopup)
  {
    if (IsInnerWindow() && !mOuterWindow) {
      NS_ERROR("SetPopupSpamWindow() called on inner window w/o an outer!");

      return;
    }

    GetOuterWindowInternal()->mIsPopupSpam = aPopup;
  }

  // Window Control Functions
  nsresult OpenSecurityCheck(const nsAString& aUrl, PRBool aDialog,
                             nsXPIDLCString& aOutUrl);
  NS_IMETHOD OpenInternal(const nsXPIDLCString& aUrl,
                          const nsAString& aName,
                          const nsAString& aOptions,
                          PRBool aDialog, jsval *argv, PRUint32 argc,
                          nsISupports *aExtraArgument, nsIDOMWindow **aReturn);
  static void CloseWindow(nsISupports* aWindow);
  static void ClearWindowScope(nsISupports* aWindow);

  // Timeout Functions
  nsresult SetTimeoutOrInterval(PRBool aIsInterval, PRInt32* aReturn);
  void RunTimeout(nsTimeout *aTimeout);
  nsresult ClearTimeoutOrInterval();
  void ClearAllTimeouts();
  void InsertTimeoutIntoList(nsTimeout **aInsertionPoint, nsTimeout *aTimeout);
  static void TimerCallback(nsITimer *aTimer, void *aClosure);

  // Helper Functions
  nsresult GetTreeOwner(nsIDocShellTreeOwner** aTreeOwner);
  nsresult GetTreeOwner(nsIBaseWindow** aTreeOwner);
  nsresult GetWebBrowserChrome(nsIWebBrowserChrome** aBrowserChrome);
  // GetScrollInfo does not flush.  Callers should do it themselves as needed,
  // depending on which info they actually want off the scrollable view.
  nsresult GetScrollInfo(nsIScrollableView** aScrollableView, float* aP2T,
                         float* aT2P);
  nsresult SecurityCheckURL(const char *aURL);
  nsresult BuildURIfromBase(const char *aURL,
                            nsIURI **aBuiltURI,
                            PRBool *aFreeSecurityPass, JSContext **aCXused);
  PopupControlState CheckForAbusePoint();
  OpenAllowValue CheckOpenAllow(PopupControlState aAbuseLevel,
                                const nsAString &aName);
  void     FireAbuseEvents(PRBool aBlocked, PRBool aWindow,
                           const nsAString &aPopupURL,
                           const nsAString &aPopupWindowFeatures);

  void FlushPendingNotifications(mozFlushType aType);
  void EnsureSizeUpToDate();
  void EnsureReflowFlushAndPaint();
  nsresult CheckSecurityWidthAndHeight(PRInt32* width, PRInt32* height);
  nsresult CheckSecurityLeftAndTop(PRInt32* left, PRInt32* top);
  static PRBool CanSetProperty(const char *aPrefName);

  static void MakeScriptDialogTitle(const nsAString &aInTitle,
                                    nsAString &aOutTitle);

  // Helper for window.find()
  nsresult FindInternal(const nsAString& aStr, PRBool caseSensitive,
                       PRBool backwards, PRBool wrapAround, PRBool wholeWord, 
                       PRBool searchInFrames, PRBool showDialog, 
                       PRBool *aReturn);

  nsresult ConvertCharset(const nsAString& aStr, char** aDest);

  PRBool   GetBlurSuppression();

  // If aDoFlush is true, we'll flush our own layout; otherwise we'll try to
  // just flush our parent and only flush ourselves if we think we need to.
  nsresult GetScrollXY(PRInt32* aScrollX, PRInt32* aScrollY,
                       PRBool aDoFlush);
  nsresult GetScrollMaxXY(PRInt32* aScrollMaxX, PRInt32* aScrollMaxY);

  PRBool IsFrame()
  {
    return GetParentInternal() != nsnull;
  }

  PRBool DispatchCustomEvent(const char *aEventName);

  PRBool WindowExists(const nsAString& aName);

  already_AddRefed<nsIWidget> GetMainWidget();

  void SuspendTimeouts();
  virtual nsresult ResumeTimeouts();

  void Freeze()
  {
    NS_ASSERTION(!IsFrozen(), "Double-freezing?");
    mIsFrozen = PR_TRUE;
  }

  void Thaw()
  {
    mIsFrozen = PR_FALSE;
  }

  // When adding new member variables, be careful not to create cycles
  // through JavaScript.  If there is any chance that a member variable
  // could own objects that are implemented in JavaScript, then those
  // objects will keep the global object (this object) alive.  To prevent
  // these cycles, ownership of such members must be released in
  // |CleanUp| and |SetDocShell|.

  // This member is also used on both inner and outer windows, but
  // for slightly different purposes. On inner windows it means the
  // inner window is held onto by session history and should not
  // change. On outer windows it means that the window is in a state
  // where we don't want to force creation of a new inner window since
  // we're in the middle of doing just that.
  PRPackedBool                  mIsFrozen : 1;

  // These members are only used on outer window objects. Make sure
  // you never set any of these on an inner object!
  PRPackedBool                  mFullScreen : 1;
  PRPackedBool                  mIsClosed : 1;
  PRPackedBool                  mInClose : 1;
  // mHavePendingClose means we've got a termination function set to
  // close us when the JS stops executing or that we have a close
  // event posted.  If this is set, just ignore window.close() calls.
  PRPackedBool                  mHavePendingClose : 1;
  PRPackedBool                  mOpenerWasCleared : 1;
  PRPackedBool                  mIsPopupSpam : 1;

  nsCOMPtr<nsIScriptContext>    mContext;
  nsCOMPtr<nsIDOMWindowInternal> mOpener;
  nsCOMPtr<nsIControllers>      mControllers;
  JSObject*                     mArguments;
  nsRefPtr<nsNavigator>         mNavigator;
  nsRefPtr<nsScreen>            mScreen;
  nsRefPtr<nsHistory>           mHistory;
  nsRefPtr<nsDOMWindowList>     mFrames;
  nsRefPtr<nsBarProp>           mMenubar;
  nsRefPtr<nsBarProp>           mToolbar;
  nsRefPtr<nsBarProp>           mLocationbar;
  nsRefPtr<nsBarProp>           mPersonalbar;
  nsRefPtr<nsBarProp>           mStatusbar;
  nsRefPtr<nsBarProp>           mScrollbars;
  nsCOMPtr<nsIWeakReference>    mWindowUtils;
  nsRefPtr<nsLocation>          mLocation;
  nsString                      mStatus;
  nsString                      mDefaultStatus;

  nsIScriptGlobalObjectOwner*   mGlobalObjectOwner; // Weak Reference
  nsIDocShell*                  mDocShell;  // Weak Reference
  nsCOMPtr<nsIDOMCrypto>        mCrypto;
  nsCOMPtr<nsIDOMPkcs11>        mPkcs11;

  nsCOMPtr<nsIXPConnectJSObjectHolder> mInnerWindowHolder;
  nsCOMPtr<nsIPrincipal> mOpenerScriptPrincipal; // strong; used to determine
                                                 // whether to clear scope

  // These member variable are used only on inner windows.
  nsCOMPtr<nsIEventListenerManager> mListenerManager;
  nsTimeout*                    mTimeouts;
  nsTimeout**                   mTimeoutInsertionPoint;
  PRUint32                      mTimeoutPublicIdCounter;
  PRUint32                      mTimeoutFiringDepth;

  // These member variables are used on both inner and the outer windows.
  nsCOMPtr<nsIPrincipal> mDocumentPrincipal;
  JSObject* mJSObject;

  friend class nsDOMScriptableHelper;
  friend class nsDOMWindowUtils;
  static nsIScriptSecurityManager *sSecMan;
  static nsIFactory *sComputedDOMStyleFactory;
};

/*
 * nsGlobalChromeWindow inherits from nsGlobalWindow. It is the global
 * object created for a Chrome Window only.
 */
class nsGlobalChromeWindow : public nsGlobalWindow,
                             public nsIDOMChromeWindow
{
public:
  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMChromeWindow interface
  NS_DECL_NSIDOMCHROMEWINDOW

  nsGlobalChromeWindow(nsGlobalWindow *aOuterWindow)
    : nsGlobalWindow(aOuterWindow)
  {
  }

protected:
  nsCOMPtr<nsIBrowserDOMWindow> mBrowserDOMWindow;
};

/*
 * Timeout struct that holds information about each JavaScript
 * timeout.
 */
struct nsTimeout
{
  nsTimeout()
  {
#ifdef DEBUG_jst
    {
      extern int gTimeoutCnt;

      ++gTimeoutCnt;
    }
#endif

    memset(this, 0, sizeof(*this));

    MOZ_COUNT_CTOR(nsTimeout);
  }

  ~nsTimeout()
  {
#ifdef DEBUG_jst
    {
      extern int gTimeoutCnt;

      --gTimeoutCnt;
    }
#endif

    MOZ_COUNT_DTOR(nsTimeout);
  }

  void Release(nsIScriptContext* aContext);
  void AddRef();

  // Window for which this timeout fires
  nsGlobalWindow *mWindow;

  // The JS expression to evaluate or function to call, if !mExpr
  JSString *mExpr;
  JSObject *mFunObj;

  // The actual timer object
  nsCOMPtr<nsITimer> mTimer;

  // Function actual arguments and argument count
  jsval *mArgv;
  PRUint16 mArgc;

  // True if the timeout was cleared
  PRPackedBool mCleared;

  // True if this is one of the timeouts that are currently running
  PRPackedBool mRunning;

  // Returned as value of setTimeout()
  PRUint32 mPublicId;

  // Non-zero interval in milliseconds if repetitive timeout
  PRInt32 mInterval;

  // Nominal time (in microseconds since the epoch) to run this
  // timeout
  PRTime mWhen;

  // Principal with which to execute
  nsCOMPtr<nsIPrincipal> mPrincipal;

  // filename, line number and JS language version string of the
  // caller of setTimeout()
  char *mFileName;
  PRUint32 mLineNo;
  const char *mVersion;

  // stack depth at which timeout is firing
  PRUint32 mFiringDepth;

  // Pointer to the next timeout in the linked list of scheduled
  // timeouts
  nsTimeout *mNext;

  // The popup state at timeout creation time if not created from
  // another timeout
  PopupControlState mPopupState;

private:
  // reference count for shared usage
  PRInt32 mRefCnt;
};

//*****************************************************************************
// nsNavigator: Script "navigator" object
//*****************************************************************************

class nsNavigator : public nsIDOMNavigator,
                    public nsIDOMJSNavigator
{
public:
  nsNavigator(nsIDocShell *aDocShell);
  virtual ~nsNavigator();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMNAVIGATOR
  NS_DECL_NSIDOMJSNAVIGATOR
  
  void SetDocShell(nsIDocShell *aDocShell);
  nsIDocShell *GetDocShell()
  {
    return mDocShell;
  }

  void LoadingNewDocument();
  nsresult RefreshMIMEArray();

protected:
  nsRefPtr<nsMimeTypeArray> mMimeTypes;
  nsRefPtr<nsPluginArray> mPlugins;
  nsIDocShell* mDocShell; // weak reference

  static jsval       sPrefInternal_id;
};

class nsIURI;

//*****************************************************************************
// nsLocation: Script "location" object
//*****************************************************************************

class nsLocation : public nsIDOMLocation,
                   public nsIDOMNSLocation
{
public:
  nsLocation(nsIDocShell *aDocShell);
  virtual ~nsLocation();

  NS_DECL_ISUPPORTS

  void SetDocShell(nsIDocShell *aDocShell);
  nsIDocShell *GetDocShell();

  // nsIDOMLocation
  NS_DECL_NSIDOMLOCATION

  // nsIDOMNSLocation
  NS_DECL_NSIDOMNSLOCATION

protected:
  // In the case of jar: uris, we sometimes want the place the jar was
  // fetched from as the URI instead of the jar: uri itself.  Pass in
  // PR_TRUE for aGetInnermostURI when that's the case.
  nsresult GetURI(nsIURI** aURL, PRBool aGetInnermostURI = PR_FALSE);
  nsresult GetWritableURI(nsIURI** aURL);
  nsresult SetURI(nsIURI* aURL, PRBool aReplace = PR_FALSE);
  nsresult SetHrefWithBase(const nsAString& aHref, nsIURI* aBase,
                           PRBool aReplace);
  nsresult SetHrefWithContext(JSContext* cx, const nsAString& aHref,
                              PRBool aReplace);

  nsresult GetSourceBaseURL(JSContext* cx, nsIURI** sourceURL);
  nsresult GetSourceDocument(JSContext* cx, nsIDocument** aDocument);

  nsresult CheckURL(nsIURI *url, nsIDocShellLoadInfo** aLoadInfo);
  nsresult FindUsableBaseURI(nsIURI * aBaseURI, nsIDocShell * aParent, nsIURI ** aUsableURI);

  nsWeakPtr mDocShell;
};

/* factory function */
nsresult NS_NewScriptGlobalObject(PRBool aIsChrome,
                                  nsIScriptGlobalObject **aResult);

#endif /* nsGlobalWindow_h___ */
