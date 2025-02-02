/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

/* A namespace class for static content utilities. */

#ifndef nsContentUtils_h___
#define nsContentUtils_h___

#include "jspubtd.h"
#include "nsAString.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsIJSContextStack.h"
#include "nsIScriptContext.h"
#include "nsCOMArray.h"
#include "nsIStatefulFrame.h"
#include "nsIPref.h"
#include "nsINodeInfo.h"
#include "nsNodeInfoManager.h"
#include "nsContentList.h"
#include "nsVoidArray.h"

class nsIXPConnect;
class nsIContent;
class nsIDOMNode;
class nsIDocument;
class nsIDocShell;
class nsINameSpaceManager;
class nsIScriptSecurityManager;
class nsIThreadJSContextStack;
class nsIParserService;
class nsIIOService;
class nsIURI;
class imgIDecoderObserver;
class imgIRequest;
class imgILoader;
class nsIPrefBranch;
class nsIImage;
class nsIImageLoadingContent;
class nsIDOMHTMLFormElement;
class nsIDOMDocument;
class nsIConsoleService;
class nsIStringBundleService;
class nsIStringBundle;
class nsIJSRuntimeService;
class nsIScriptGlobalObject;
struct JSRuntime;
#ifdef MOZ_XTF
class nsIXTFService;
#endif

class nsContentUtils
{
public:
  static nsresult Init();

  // You MUST pass the old ownerDocument of aContent in as aOldDocument and the
  // new one as aNewDocument.  aNewParent is allowed to be null; in that case
  // aNewDocument will be assumed to be the parent.  Note that at this point
  // the actual ownerDocument of aContent may not yet be aNewDocument.
  // XXXbz but then if it gets wrapped after we do this call but before its
  // ownerDocument actually changes, things will break...
  static nsresult ReparentContentWrapper(nsIContent *aContent,
                                         nsIContent *aNewParent,
                                         nsIDocument *aNewDocument,
                                         nsIDocument *aOldDocument);

  /**
   * When a document's scope changes (e.g., from document.open(), call this
   * function to move all content wrappers from the old scope to the new one.
   */
  static nsresult ReparentContentWrappersInScope(nsIScriptGlobalObject *aOldScope,
                                                 nsIScriptGlobalObject *aNewScope);

  static PRBool   IsCallerChrome();

  static PRBool   IsCallerTrustedForRead();

  static PRBool   IsCallerTrustedForWrite();

  /*
   * Returns true if the nodes are both in the same document or
   * if neither is in a document.
   * Returns false if the nodes are not in the same document.
   */
  static PRBool   InSameDoc(nsIDOMNode *aNode,
                            nsIDOMNode *aOther);

  /**
   * Do not ever pass null pointers to this method.  If one of your
   * nsIContents is null, you have to decide for yourself what
   * "IsDescendantOf" really means.
   *
   * @param  aPossibleDescendant node to test for being a descendant of
   *         aPossibleAncestor
   * @param  aPossibleAncestor node to test for being an ancestor of
   *         aPossibleDescendant
   * @return PR_TRUE if aPossibleDescendant is a descendant of
   *         aPossibleAncestor (or is aPossibleAncestor).  PR_FALSE
   *         otherwise.
   */
  static PRBool ContentIsDescendantOf(nsIContent* aPossibleDescendant,
                                      nsIContent* aPossibleAncestor);

  /*
   * This method fills the |aArray| with all ancestor nodes of |aNode|
   * including |aNode| at the zero index.
   *
   * These elements were |nsIDOMNode*|s before casting to |void*| and must
   * be cast back to |nsIDOMNode*| on usage, or bad things will happen.
   */
  static nsresult GetAncestors(nsIDOMNode* aNode,
                               nsVoidArray* aArray);

  /*
   * This method fills |aAncestorNodes| with all ancestor nodes of |aNode|
   * including |aNode| (QI'd to nsIContent) at the zero index.
   * For each ancestor, there is a corresponding element in |aAncestorOffsets|
   * which is the IndexOf the child in relation to its parent.
   *
   * The elements of |aAncestorNodes| were |nsIContent*|s before casting to
   * |void*| and must be cast back to |nsIContent*| on usage, or bad things
   * will happen.
   *
   * This method just sucks.
   */
  static nsresult GetAncestorsAndOffsets(nsIDOMNode* aNode,
                                         PRInt32 aOffset,
                                         nsVoidArray* aAncestorNodes,
                                         nsVoidArray* aAncestorOffsets);

  /*
   * The out parameter, |aCommonAncestor| will be the closest node, if any,
   * to both |aNode| and |aOther| which is also an ancestor of each.
   */
  static nsresult GetCommonAncestor(nsIDOMNode *aNode,
                                    nsIDOMNode *aOther,
                                    nsIDOMNode** aCommonAncestor);

  /*
   * |aDifferentNodes| will contain up to 3 elements.
   * The first, if present, is the common ancestor of |aNode| and |aOther|.
   * The second, if present, is the ancestor node of |aNode| which is
   * closest to the common ancestor, but not an ancestor of |aOther|.
   * The third, if present, is the ancestor node of |aOther| which is
   * closest to the common ancestor, but not an ancestor of |aNode|.
   *
   * @throws NS_ERROR_FAILURE if aNode and aOther are disconnected.
   */
  static nsresult GetFirstDifferentAncestors(nsIDOMNode *aNode,
                                             nsIDOMNode *aOther,
                                             nsCOMArray<nsIDOMNode>& aDifferentNodes);

  /**
   * Compares the document position of nodes which may have parents.
   * DO NOT pass in nodes that cannot have a parentNode. In other words:
   * DO NOT pass in Attr, Document, DocumentFragment, Entity, or Notation!
   * The results will be completely wrong!
   *
   * @param   aNode   The node to which you are comparing.
   * @param   aOther  The reference node to which aNode is compared.
   *
   * @return  The document position flags of the nodes.
   *
   * @see nsIDOMNode
   * @see nsIDOM3Node
   */
  static PRUint16 ComparePositionWithAncestors(nsIDOMNode *aNode,
                                               nsIDOMNode *aOther);

  /**
   * Reverses the document position flags passed in.
   *
   * @param   aDocumentPosition   The document position flags to be reversed.
   *
   * @return  The reversed document position flags.
   *
   * @see nsIDOMNode
   * @see nsIDOM3Node
   */
  static PRUint16 ReverseDocumentPosition(PRUint16 aDocumentPosition);

  static PRUint32 CopyNewlineNormalizedUnicodeTo(const nsAString& aSource,
                                                 PRUint32 aSrcOffset,
                                                 PRUnichar* aDest,
                                                 PRUint32 aLength,
                                                 PRBool& aLastCharCR);

  static PRUint32 CopyNewlineNormalizedUnicodeTo(nsReadingIterator<PRUnichar>& aSrcStart, const nsReadingIterator<PRUnichar>& aSrcEnd, nsAString& aDest);

  static nsISupports *
  GetClassInfoInstance(nsDOMClassInfoID aID);

  static const nsDependentSubstring TrimCharsInSet(const char* aSet,
                                                   const nsAString& aValue);

  static const nsDependentSubstring TrimWhitespace(const nsAString& aStr,
                                                   PRBool aTrimTrailing = PR_TRUE);

  static void Shutdown();

  /**
   * Checks whether two nodes come from the same origin. aTrustedNode is
   * considered 'safe' in that a user can operate on it and that it isn't
   * a js-object that implements nsIDOMNode.
   * Never call this function with the first node provided by script, it
   * must always be known to be a 'real' node!
   */
  static nsresult CheckSameOrigin(nsIDOMNode* aTrustedNode,
                                  nsIDOMNode* aUnTrustedNode);

  // Check if the (JS) caller can access aNode.
  static PRBool CanCallerAccess(nsIDOMNode *aNode);

  /**
   * Get the docshell through the JS context that's currently on the stack.
   * If there's no JS context currently on the stack aDocShell will be null.
   *
   * @param aDocShell The docshell or null if no JS context
   */
  static nsIDocShell *GetDocShellFromCaller();

  /**
   * The two GetDocumentFrom* functions below allow a caller to get at a
   * document that is relevant to the currently executing script.
   *
   * GetDocumentFromCaller gets its document by looking at the last called
   * function and finding the document that the function itself relates to.
   * For example, consider two windows A and B in the same origin. B has a
   * function which does something that ends up needing the current document.
   * If a script in window A were to call B's function, GetDocumentFromCaller
   * would find that function (in B) and return B's document.
   *
   * GetDocumentFromContext gets its document by looking at the currently
   * executing context's global object and returning its document. Thus,
   * given the example above, GetDocumentFromCaller would see that the
   * currently executing script was in window A, and return A's document.
   */
  /**
   * Get the document from the currently executing function. This will return
   * the document that the currently executing function is in/from.
   *
   * @return The document or null if no JS Context.
   */
  static nsIDOMDocument *GetDocumentFromCaller();

  /**
   * Get the document through the JS context that's currently on the stack.
   * If there's no JS context currently on the stack it will return null.
   * This will return the document of the calling script.
   *
   * @return The document or null if no JS context
   */
  static nsIDOMDocument *GetDocumentFromContext();

  // Check if a node is in the document prolog, i.e. before the document
  // element.
  static PRBool InProlog(nsIDOMNode *aNode);

  static nsIParserService* GetParserServiceWeakRef();

  static nsINameSpaceManager* GetNSManagerWeakRef()
  {
    return sNameSpaceManager;
  }

  static nsIIOService* GetIOServiceWeakRef()
  {
    return sIOService;
  }

  static imgILoader* GetImgLoader()
  {
    return sImgLoader;
  }

#ifdef MOZ_XTF
  static nsIXTFService* GetXTFServiceWeakRef();
#endif

  static nsIScriptSecurityManager* GetSecurityManager()
  {
    return sSecurityManager;
  }

  static nsresult GenerateStateKey(nsIContent* aContent,
                                   nsIDocument* aDocument,
                                   nsIStatefulFrame::SpecialStateID aID,
                                   nsACString& aKey);

  /**
   * Create a new nsIURI from aSpec, using aBaseURI as the base.  The
   * origin charset of the new nsIURI will be the document charset of
   * aDocument.
   */
  static nsresult NewURIWithDocumentCharset(nsIURI** aResult,
                                            const nsAString& aSpec,
                                            nsIDocument* aDocument,
                                            nsIURI* aBaseURI);

  /**
   * Determine whether aContent is in some way associated with aForm.  If the
   * form is a container the only elements that are considered to be associated
   * with a form are the elements that are contained within the form. If the
   * form is a leaf element then all elements will be accepted into this list,
   * since this can happen due to content fixup when a form spans table rows or
   * table cells.
   */
  static PRBool BelongsInForm(nsIDOMHTMLFormElement *aForm,
                              nsIContent *aContent);

  static nsresult CheckQName(const nsAString& aQualifiedName,
                             PRBool aNamespaceAware = PR_TRUE);

  static nsresult SplitQName(nsIContent* aNamespaceResolver,
                             const nsAFlatString& aQName,
                             PRInt32 *aNamespace, nsIAtom **aLocalName);

  static nsresult LookupNamespaceURI(nsIContent* aNamespaceResolver,
                                     const nsAString& aNamespacePrefix,
                                     nsAString& aNamespaceURI);

  static nsresult GetNodeInfoFromQName(const nsAString& aNamespaceURI,
                                       const nsAString& aQualifiedName,
                                       nsNodeInfoManager* aNodeInfoManager,
                                       nsINodeInfo** aNodeInfo);

  static void SplitExpatName(const PRUnichar *aExpatName, nsIAtom **aPrefix,
                             nsIAtom **aTagName, PRInt32 *aNameSpaceID);

  static nsAdoptingCString GetCharPref(const char *aPref);
  static PRPackedBool GetBoolPref(const char *aPref,
                                  PRBool aDefault = PR_FALSE);
  static PRInt32 GetIntPref(const char *aPref, PRInt32 aDefault = 0);
  static nsAdoptingString GetLocalizedStringPref(const char *aPref);
  static nsAdoptingString GetStringPref(const char *aPref);
  static void RegisterPrefCallback(const char *aPref,
                                   PrefChangedFunc aCallback,
                                   void * aClosure);
  static void UnregisterPrefCallback(const char *aPref,
                                     PrefChangedFunc aCallback,
                                     void * aClosure);
  static nsIPrefBranch *GetPrefBranch()
  {
    return sPrefBranch;
  }

  static nsresult GetDocumentAndPrincipal(nsIDOMNode* aNode,
                                          nsIDocument** aDocument,
                                          nsIPrincipal** aPrincipal);

  /**
   * Method to do security and content policy checks on the image URI
   *
   * @param aURI uri of the image to be loaded
   * @param aContext the context the image is loaded in (eg an element)
   * @param aLoadingDocument the document we belong to
   * @param aImageBlockingStatus the nsIContentPolicy blocking status for this
   *        image.  This will be set even if a security check fails for the
   *        image, to some reasonable REJECT_* value.  This out param will only
   *        be set if it's non-null.
   * @return PR_TRUE if the load can proceed, or PR_FALSE if it is blocked.
   *         Note that aImageBlockingStatus, if set will always be an ACCEPT
   *         status if PR_TRUE is returned and always be a REJECT_* status if
   *         PR_FALSE is returned.
   */
  static PRBool CanLoadImage(nsIURI* aURI,
                             nsISupports* aContext,
                             nsIDocument* aLoadingDocument,
                             PRInt16* aImageBlockingStatus = nsnull);
  /**
   * Method to start an image load.  This does not do any security checks.
   *
   * @param aURI uri of the image to be loaded
   * @param aLoadingDocument the document we belong to
   * @param aObserver the observer for the image load
   * @param aLoadFlags the load flags to use.  See nsIRequest
   * @return the imgIRequest for the image load
   */
  static nsresult LoadImage(nsIURI* aURI,
                            nsIDocument* aLoadingDocument,
                            nsIURI* aReferrer,
                            imgIDecoderObserver* aObserver,
                            PRInt32 aLoadFlags,
                            imgIRequest** aRequest);

  /**
   * Method to get an nsIImage from an image loading content
   *
   * @param aContent The image loading content.  Must not be null.
   * @param aRequest The image request [out]
   * @return the nsIImage corresponding to the first frame of the image
   */
  static already_AddRefed<nsIImage> GetImageFromContent(nsIImageLoadingContent* aContent, imgIRequest **aRequest = nsnull);

  /**
   * Method that decides whether a content node is draggable
   *
   * @param aContent The content node to test.
   * @return whether it's draggable
   */
  static PRBool ContentIsDraggable(nsIContent* aContent) {
    return IsDraggableImage(aContent) || IsDraggableLink(aContent);
  }

  /**
   * Method that decides whether a content node is a draggable image
   *
   * @param aContent The content node to test.
   * @return whether it's a draggable image
   */
  static PRBool IsDraggableImage(nsIContent* aContent);

  /**
   * Method that decides whether a content node is a draggable link
   *
   * @param aContent The content node to test.
   * @return whether it's a draggable link
   */
  static PRBool IsDraggableLink(nsIContent* aContent);

  /**
   * Method that gets the URI of the link content.  If the content
   * isn't a link, return null.
   *
   * @param aContent The link content
   * @return the URI the link points to
   */
  static already_AddRefed<nsIURI> GetLinkURI(nsIContent* aContent);

  /**
   * Method that gets the XLink uri for a content node, if it's an XLink
   *
   * @param aContent The content node, possibly an XLink
   * @return Null if aContent is not an XLink, the URI it points to otherwise
   */
  static already_AddRefed<nsIURI> GetXLinkURI(nsIContent* aContent);

  /**
   * Convenience method to create a new nodeinfo that differs only by name
   * from aNodeInfo.
   */
  static nsresult NameChanged(nsINodeInfo *aNodeInfo, nsIAtom *aName,
                              nsINodeInfo** aResult)
  {
    nsNodeInfoManager *niMgr = aNodeInfo->NodeInfoManager();

    return niMgr->GetNodeInfo(aName, aNodeInfo->GetPrefixAtom(),
                              aNodeInfo->NamespaceID(), aResult);
  }

  /**
   * Convenience method to create a new nodeinfo that differs only by prefix
   * from aNodeInfo.
   */
  static nsresult PrefixChanged(nsINodeInfo *aNodeInfo, nsIAtom *aPrefix,
                                nsINodeInfo** aResult)
  {
    nsNodeInfoManager *niMgr = aNodeInfo->NodeInfoManager();

    return niMgr->GetNodeInfo(aNodeInfo->NameAtom(), aPrefix,
                              aNodeInfo->NamespaceID(), aResult);
  }

  /**
   * Retrieve a pointer to the document that owns aNodeInfo.
   */
  static nsIDocument *GetDocument(nsINodeInfo *aNodeInfo)
  {
    return aNodeInfo->NodeInfoManager()->GetDocument();
  }

  /**
   * Returns the appropriate event argument name for the specified
   * namespace.  Added because we need to switch between SVG's "evt"
   * and the rest of the world's "event".
   */
  static const char *GetEventArgName(PRInt32 aNameSpaceID);

  /**
   * Return the nsIXPConnect service.
   */
  static nsIXPConnect *XPConnect()
  {
    return sXPConnect;
  }

  /**
   * Report a localized error message to the error console.
   *   @param aFile Properties file containing localized message.
   *   @param aMessageName Name of localized message.
   *   @param aParams Parameters to be substituted into localized message.
   *   @param aParamsLength Length of aParams.
   *   @param aURI URI of resource containing error (may be null).
   *   @param aSourceLine The text of the line that contains the error (may be
              empty).
   *   @param aLineNumber Line number within resource containing error.
   *   @param aColumnNumber Column number within resource containing error.
   *   @param aErrorFlags See nsIScriptError.
   *   @param aCategory Name of module reporting error.
   */
  enum PropertiesFile {
    eCSS_PROPERTIES,
    eXBL_PROPERTIES,
    eXUL_PROPERTIES,
    eLAYOUT_PROPERTIES,
    eFORMS_PROPERTIES,
    ePRINTING_PROPERTIES,
    eDOM_PROPERTIES,
    eBRAND_PROPERTIES,
    PropertiesFile_COUNT
  };
  static nsresult ReportToConsole(PropertiesFile aFile,
                                  const char *aMessageName,
                                  const PRUnichar **aParams,
                                  PRUint32 aParamsLength,
                                  nsIURI* aURI,
                                  const nsAFlatString& aSourceLine,
                                  PRUint32 aLineNumber,
                                  PRUint32 aColumnNumber,
                                  PRUint32 aErrorFlags,
                                  const char *aCategory);

  /**
   * Get the localized string named |aKey| in properties file |aFile|.
   */
  static nsresult GetLocalizedString(PropertiesFile aFile,
                                     const char* aKey,
                                     nsXPIDLString& aResult);

  /**
   * Fill (with the parameters given) the localized string named |aKey| in
   * properties file |aFile|.
   */
  static nsresult FormatLocalizedString(PropertiesFile aFile,
                                        const char* aKey,
                                        const PRUnichar **aParams,
                                        PRUint32 aParamsLength,
                                        nsXPIDLString& aResult);

  /**
   * Returns a list containing all elements in the document that are
   * of type nsIContent::eHTML_FORM_CONTROL.
   */
  static already_AddRefed<nsContentList>
  GetFormControlElements(nsIDocument *aDocument);

  /**
   * Returns true if aDocument is a chrome document
   */
  static PRBool IsChromeDoc(nsIDocument *aDocument);

  /**
   * Notify XPConnect if an exception is pending on aCx.
   */
  static void NotifyXPCIfExceptionPending(JSContext *aCx);

  /**
   * Release *aSupportsPtr when the shutdown notification is received
   */
  static nsresult ReleasePtrOnShutdown(nsISupports** aSupportsPtr) {
    NS_ASSERTION(aSupportsPtr, "Expect to crash!");
    NS_ASSERTION(*aSupportsPtr, "Expect to crash!");
    return sPtrsToPtrsToRelease->AppendElement(aSupportsPtr) ? NS_OK :
      NS_ERROR_OUT_OF_MEMORY;
  }
  /**
   * Make sure that whatever value *aPtr contains at any given moment is
   * protected from JS GC until we remove the GC root.  A call to this that
   * succeeds MUST be matched by a call to RemoveJSGCRoot to avoid leaking.
   */
  static nsresult AddJSGCRoot(jsval* aPtr, const char* aName) {
    return AddJSGCRoot((void*)aPtr, aName);
  }

  /**
   * Make sure that whatever object *aPtr is pointing to at any given moment is
   * protected from JS GC until we remove the GC root.  A call to this that
   * succeeds MUST be matched by a call to RemoveJSGCRoot to avoid leaking.
   */
  static nsresult AddJSGCRoot(JSObject** aPtr, const char* aName) {
    return AddJSGCRoot((void*)aPtr, aName);
  }

  /**
   * Make sure that whatever object *aPtr is pointing to at any given moment is
   * protected from JS GC until we remove the GC root.  A call to this that
   * succeeds MUST be matched by a call to RemoveJSGCRoot to avoid leaking.
   */
  static nsresult AddJSGCRoot(void* aPtr, const char* aName);  

  /**
   * Remove aPtr as a JS GC root
   */
  static nsresult RemoveJSGCRoot(jsval* aPtr) {
    return RemoveJSGCRoot((void*)aPtr);
  }
  static nsresult RemoveJSGCRoot(JSObject** aPtr) {
    return RemoveJSGCRoot((void*)aPtr);
  }
  static nsresult RemoveJSGCRoot(void* aPtr);
  
private:
  static nsresult doReparentContentWrapper(nsIContent *aChild,
                                           JSContext *cx,
                                           JSObject *aOldGlobal,
                                           JSObject *aNewGlobal);

  static nsresult EnsureStringBundle(PropertiesFile aFile);


  static nsIDOMScriptObjectFactory *sDOMScriptObjectFactory;

  static nsIXPConnect *sXPConnect;

  static nsIScriptSecurityManager *sSecurityManager;

  static nsIThreadJSContextStack *sThreadJSContextStack;

  static nsIParserService *sParserService;

  static nsINameSpaceManager *sNameSpaceManager;

  static nsIIOService *sIOService;

#ifdef MOZ_XTF
  static nsIXTFService *sXTFService;
#endif

  static nsIPrefBranch *sPrefBranch;

  static nsIPref *sPref;

  static imgILoader* sImgLoader;

  static nsIConsoleService* sConsoleService;

  static nsIStringBundleService* sStringBundleService;
  static nsIStringBundle* sStringBundles[PropertiesFile_COUNT];

  // Holds pointers to nsISupports* that should be released at shutdown
  static nsVoidArray* sPtrsToPtrsToRelease;

  // For now, we don't want to automatically clean this up in Shutdown(), since
  // consumers might unfortunately end up wanting to use it after that
  static nsIJSRuntimeService* sJSRuntimeService;
  static JSRuntime* sScriptRuntime;
  static PRInt32 sScriptRootCount;
  
  static PRBool sInitialized;
};


class nsCxPusher
{
public:
  nsCxPusher() : mScriptIsRunning(PR_FALSE) {}
  ~nsCxPusher() { Pop(); }
  // Returns PR_FALSE if something erroneous happened.
  PRBool Push(nsISupports *aCurrentTarget);
  void Pop();

private:
  nsCOMPtr<nsIJSContextStack> mStack;
  nsCOMPtr<nsIScriptContext> mScx;
  PRBool mScriptIsRunning;
};

class nsAutoGCRoot {
public:
  // aPtr should be the pointer to the jsval we want to protect
  nsAutoGCRoot(jsval* aPtr, nsresult* aResult) :
    mPtr(aPtr)
  {
    mResult = *aResult =
      nsContentUtils::AddJSGCRoot(aPtr, "nsAutoGCRoot");
  }

  // aPtr should be the pointer to the JSObject* we want to protect
  nsAutoGCRoot(JSObject** aPtr, nsresult* aResult) :
    mPtr(aPtr)
  {
    mResult = *aResult =
      nsContentUtils::AddJSGCRoot(aPtr, "nsAutoGCRoot");
  }

  // aPtr should be the pointer to the thing we want to protect
  nsAutoGCRoot(void* aPtr, nsresult* aResult) :
    mPtr(aPtr)
  {
    mResult = *aResult =
      nsContentUtils::AddJSGCRoot(aPtr, "nsAutoGCRoot");
  }

  ~nsAutoGCRoot() {
    if (NS_SUCCEEDED(mResult)) {
      nsContentUtils::RemoveJSGCRoot(mPtr);
    }
  }

private:
  void* mPtr;
  nsresult mResult;
};

#define NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(_class)                      \
  if (aIID.Equals(NS_GET_IID(nsIClassInfo))) {                                \
    foundInterface =                                                          \
      nsContentUtils::GetClassInfoInstance(eDOMClassInfo_##_class##_id);      \
    if (!foundInterface) {                                                    \
      *aInstancePtr = nsnull;                                                 \
      return NS_ERROR_OUT_OF_MEMORY;                                          \
    }                                                                         \
  } else

#define NS_INTERFACE_MAP_ENTRY_TEAROFF(_interface, _allocator)                \
  if (aIID.Equals(NS_GET_IID(_interface))) {                                  \
    foundInterface = NS_STATIC_CAST(_interface *, _allocator);                \
    if (!foundInterface) {                                                    \
      *aInstancePtr = nsnull;                                                 \
      return NS_ERROR_OUT_OF_MEMORY;                                          \
    }                                                                         \
  } else

#endif /* nsContentUtils_h___ */
