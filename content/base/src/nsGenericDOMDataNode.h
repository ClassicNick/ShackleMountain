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
#ifndef nsGenericDOMDataNode_h___
#define nsGenericDOMDataNode_h___

#include "nsITextContent.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMEventReceiver.h"
#include "nsTextFragment.h"
#include "nsVoidArray.h"
#include "nsDOMError.h"
#include "nsIEventListenerManager.h"
#include "nsGenericElement.h"


class nsIDOMAttr;
class nsIDOMEventListener;
class nsIDOMNodeList;
class nsIFrame;
class nsIDOMText;
class nsINodeInfo;
class nsURI;

#define PARENT_BIT_IS_IN_A_HASH ((PtrBits)0x1 << 1)

#define NODEINFOMANAGER_BIT_IS_NATIVE_ANONYMOUS 0x1

class nsGenericDOMDataNode : public nsITextContent
{
public:
  NS_DECL_ISUPPORTS

  nsGenericDOMDataNode(nsNodeInfoManager *aNodeInfoManager);
  virtual ~nsGenericDOMDataNode();

  // Implementation for nsIDOMNode
  nsresult GetNodeValue(nsAString& aNodeValue);
  nsresult SetNodeValue(const nsAString& aNodeValue);
  nsresult GetParentNode(nsIDOMNode** aParentNode);
  nsresult GetAttributes(nsIDOMNamedNodeMap** aAttributes)
  {
    NS_ENSURE_ARG_POINTER(aAttributes);
    *aAttributes = nsnull;
    return NS_OK;
  }
  nsresult GetPreviousSibling(nsIDOMNode** aPreviousSibling);
  nsresult GetNextSibling(nsIDOMNode** aNextSibling);
  nsresult GetChildNodes(nsIDOMNodeList** aChildNodes);
  nsresult HasChildNodes(PRBool* aHasChildNodes)
  {
    NS_ENSURE_ARG_POINTER(aHasChildNodes);
    *aHasChildNodes = PR_FALSE;
    return NS_OK;
  }
  nsresult HasAttributes(PRBool* aHasAttributes)
  {
    NS_ENSURE_ARG_POINTER(aHasAttributes);
    *aHasAttributes = PR_FALSE;
    return NS_OK;
  }
  nsresult GetFirstChild(nsIDOMNode** aFirstChild)
  {
    NS_ENSURE_ARG_POINTER(aFirstChild);
    *aFirstChild = nsnull;
    return NS_OK;
  }
  nsresult GetLastChild(nsIDOMNode** aLastChild)
  {
    NS_ENSURE_ARG_POINTER(aLastChild);
    *aLastChild = nsnull;
    return NS_OK;
  }
  nsresult InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild,
                        nsIDOMNode** aReturn)
  {
    NS_ENSURE_ARG_POINTER(aReturn);
    *aReturn = nsnull;
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }
  nsresult ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild,
                        nsIDOMNode** aReturn)
  {
    NS_ENSURE_ARG_POINTER(aReturn);
    *aReturn = nsnull;

    /*
     * Data nodes can't have children.
     */
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }
  nsresult RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
  {
    NS_ENSURE_ARG_POINTER(aReturn);
    *aReturn = nsnull;

    /*
     * Data nodes can't have children, i.e. aOldChild can't be a child of
     * this node.
     */
    return NS_ERROR_DOM_NOT_FOUND_ERR;
  }
  nsresult AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
  {
    NS_ENSURE_ARG_POINTER(aReturn);
    *aReturn = nsnull;
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }
  nsresult GetOwnerDocument(nsIDOMDocument** aOwnerDocument);
  nsresult GetNamespaceURI(nsAString& aNamespaceURI);
  nsresult GetLocalName(nsAString& aLocalName);
  nsresult GetPrefix(nsAString& aPrefix);
  nsresult SetPrefix(const nsAString& aPrefix);
  nsresult Normalize();
  nsresult IsSupported(const nsAString& aFeature,
                       const nsAString& aVersion,
                       PRBool* aReturn);
  nsresult GetBaseURI(nsAString& aURI);
  nsresult LookupPrefix(const nsAString& aNamespaceURI,
                        nsAString& aPrefix);
  nsresult LookupNamespaceURI(const nsAString& aNamespacePrefix,
                              nsAString& aNamespaceURI);

  // Implementation for nsIDOMCharacterData
  nsresult GetData(nsAString& aData);
  nsresult SetData(const nsAString& aData);
  nsresult GetLength(PRUint32* aLength);
  nsresult SubstringData(PRUint32 aOffset, PRUint32 aCount,
                         nsAString& aReturn);
  nsresult AppendData(const nsAString& aArg);
  nsresult InsertData(PRUint32 aOffset, const nsAString& aArg);
  nsresult DeleteData(PRUint32 aOffset, PRUint32 aCount);
  nsresult ReplaceData(PRUint32 aOffset, PRUint32 aCount,
                       const nsAString& aArg);

  // Implementation for nsIContent
  nsIDocument* GetDocument() const;
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  PRBool IsInDoc() const
  {
    return mParentPtrBits & PARENT_BIT_INDOCUMENT;
  }

  nsIDocument *GetCurrentDoc() const
  {
    return IsInDoc() ? GetOwnerDoc() : nsnull;
  }

  nsIDocument *GetOwnerDoc() const
  {
    return GetNodeInfoManager()->GetDocument();
  }

  virtual PRBool IsNativeAnonymous() const;
  virtual void SetNativeAnonymous(PRBool aAnonymous);
  virtual PRInt32 GetNameSpaceID() const;
  virtual nsIAtom *GetIDAttributeName() const;
  virtual already_AddRefed<nsINodeInfo> GetExistingAttrNameFromQName(const nsAString& aStr) const;
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, PRBool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify);
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRBool aNotify);
  virtual nsresult GetAttr(PRInt32 aNameSpaceID, nsIAtom *aAttribute,
                           nsAString& aResult) const;
  virtual PRBool HasAttr(PRInt32 aNameSpaceID, nsIAtom *aAttribute) const;
  virtual nsresult GetAttrNameAt(PRUint32 aIndex, PRInt32* aNameSpaceID,
                                 nsIAtom** aName, nsIAtom** aPrefix) const;
  virtual PRUint32 GetAttrCount() const;
#ifdef DEBUG
  virtual void List(FILE* out, PRInt32 aIndent) const;
  virtual void DumpContent(FILE* out, PRInt32 aIndent, PRBool aDumpAll) const;
#endif
  virtual nsresult HandleDOMEvent(nsPresContext* aPresContext,
                                  nsEvent* aEvent, nsIDOMEvent** aDOMEvent,
                                  PRUint32 aFlags,
                                  nsEventStatus* aEventStatus);
  virtual PRUint32 ContentID() const;
  virtual nsresult RangeAdd(nsIDOMRange* aRange);
  virtual void RangeRemove(nsIDOMRange* aRange);
  virtual const nsVoidArray *GetRangeList() const;

  virtual nsIContent *GetBindingParent() const;
  virtual PRBool IsContentOfType(PRUint32 aFlags) const;

  virtual nsresult GetListenerManager(nsIEventListenerManager **aResult);
  virtual already_AddRefed<nsIURI> GetBaseURI() const;

  virtual nsINodeInfo *GetNodeInfo() const;
  virtual PRUint32 GetChildCount() const;
  virtual nsIContent *GetChildAt(PRUint32 aIndex) const;
  virtual PRInt32 IndexOf(nsIContent* aPossibleChild) const;
  virtual nsresult InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                 PRBool aNotify);
  virtual nsresult AppendChildTo(nsIContent* aKid, PRBool aNotify);
  virtual nsresult RemoveChildAt(PRUint32 aIndex, PRBool aNotify);
  virtual PRBool MayHaveFrame() const;

  // nsITextContent
  virtual const nsTextFragment *Text();
  virtual PRUint32 TextLength();
  virtual void SetText(const PRUnichar* aBuffer, PRUint32 aLength,
                       PRBool aNotify)
  {
    DoSetText(aBuffer, aLength, PR_FALSE, aNotify);
  }
  virtual void SetText(const nsAString& aStr, PRBool aNotify)
  {
    DoSetText(aStr, PR_FALSE, aNotify);
  }
  virtual void SetText(const char* aBuffer, PRUint32 aLength, PRBool aNotify)
  {
    DoSetText(aBuffer, aLength, PR_FALSE, aNotify);
  }
  virtual PRBool IsOnlyWhitespace();
  virtual void AppendTextTo(nsAString& aResult);

  //----------------------------------------

  virtual already_AddRefed<nsITextContent> CloneContent(PRBool aCloneText,
                                                        nsNodeInfoManager *aNodeInfoManager);

#ifdef DEBUG
  void ToCString(nsAString& aBuf, PRInt32 aOffset, PRInt32 aLen) const;
#endif

protected:
  nsresult SplitText(PRUint32 aOffset, nsIDOMText** aReturn);

  // Like SetText but allows to specify whether this is an append for
  // notifications.
  void DoSetText(const PRUnichar* aBuffer, PRUint32 aLength, PRBool aIsAppend,
               PRBool aNotify);
  void DoSetText(const char* aBuffer, PRUint32 aLength, PRBool aIsAppend,
               PRBool aNotify);
  void DoSetText(const nsAString& aStr, PRBool aIsAppend, PRBool aNotify);

  nsTextFragment mText;
  typedef unsigned long PtrBits;
  nsNodeInfoManager* GetNodeInfoManager() const {
    return NS_REINTERPRET_CAST(nsNodeInfoManager*, PtrBits(mNodeInfoManagerBits) &
                                                   ~NODEINFOMANAGER_BIT_IS_NATIVE_ANONYMOUS);
  }
  void SetNodeInfoManager(nsNodeInfoManager* aNodeInfoManager);

private:
  // Note: the LSB of this pointer is used for IsNativeAnonymous().
  // Use Set/GetNodeInfoManager(), not the raw pointer.
  void* mNodeInfoManagerBits;

  void LookupListenerManager(nsIEventListenerManager **aListenerManager) const;
  nsVoidArray *LookupRangeList() const;

  void SetBidiStatus();

  already_AddRefed<nsIAtom> GetCurrentValueAtom();

  void SetIsInAHash()
  {
    mParentPtrBits |= PARENT_BIT_IS_IN_A_HASH;
  }
  PRBool GetIsInAHash() const
  {
    return (mParentPtrBits & PARENT_BIT_IS_IN_A_HASH);
  }

  void SetHasRangeList()
  {
    SetIsInAHash();
  }

  void SetHasEventListenerManager()
  {
    SetIsInAHash();
  }

  PRBool CouldHaveRangeList() const
  {
    return GetIsInAHash() && nsGenericElement::sRangeListsHash.ops;
  }

  PRBool CouldHaveEventListenerManager() const
  {
    return GetIsInAHash() && nsGenericElement::sEventListenerManagersHash.ops;
  }
};

//----------------------------------------------------------------------

/**
 * Mostly implement the nsIDOMNode API by forwarding the methods to
 * nsGenericDOMDataNode
 *
 * Note that classes using this macro will need to implement:
 *       NS_IMETHOD GetNodeType(PRUint16* aNodeType);
 *       NS_IMETHOD CloneNode(PRBool aDeep, nsIDOMNode** aReturn);
 */
#define NS_IMPL_NSIDOMNODE_USING_GENERIC_DOM_DATA                           \
  NS_IMETHOD GetNodeName(nsAString& aNodeName);                             \
  NS_IMETHOD GetLocalName(nsAString& aLocalName) {                          \
    return nsGenericDOMDataNode::GetLocalName(aLocalName);                  \
  }                                                                         \
  NS_IMETHOD GetNodeValue(nsAString& aNodeValue);                           \
  NS_IMETHOD SetNodeValue(const nsAString& aNodeValue);                     \
  NS_IMETHOD GetNodeType(PRUint16* aNodeType);                              \
  NS_IMETHOD GetParentNode(nsIDOMNode** aParentNode) {                      \
    return nsGenericDOMDataNode::GetParentNode(aParentNode);                \
  }                                                                         \
  NS_IMETHOD GetChildNodes(nsIDOMNodeList** aChildNodes) {                  \
    return nsGenericDOMDataNode::GetChildNodes(aChildNodes);                \
  }                                                                         \
  NS_IMETHOD HasChildNodes(PRBool* aHasChildNodes) {                        \
    return nsGenericDOMDataNode::HasChildNodes(aHasChildNodes);             \
  }                                                                         \
  NS_IMETHOD HasAttributes(PRBool* aHasAttributes) {                        \
    return nsGenericDOMDataNode::HasAttributes(aHasAttributes);             \
  }                                                                         \
  NS_IMETHOD GetFirstChild(nsIDOMNode** aFirstChild) {                      \
    return nsGenericDOMDataNode::GetFirstChild(aFirstChild);                \
  }                                                                         \
  NS_IMETHOD GetLastChild(nsIDOMNode** aLastChild) {                        \
    return nsGenericDOMDataNode::GetLastChild(aLastChild);                  \
  }                                                                         \
  NS_IMETHOD GetPreviousSibling(nsIDOMNode** aPreviousSibling) {            \
    return nsGenericDOMDataNode::GetPreviousSibling(aPreviousSibling);      \
  }                                                                         \
  NS_IMETHOD GetNextSibling(nsIDOMNode** aNextSibling) {                    \
    return nsGenericDOMDataNode::GetNextSibling(aNextSibling);              \
  }                                                                         \
  NS_IMETHOD GetAttributes(nsIDOMNamedNodeMap** aAttributes) {              \
    return nsGenericDOMDataNode::GetAttributes(aAttributes);                \
  }                                                                         \
  NS_IMETHOD InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild,     \
                             nsIDOMNode** aReturn) {                        \
    return nsGenericDOMDataNode::InsertBefore(aNewChild, aRefChild,         \
                                              aReturn);                     \
  }                                                                         \
  NS_IMETHOD AppendChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn) {     \
    return nsGenericDOMDataNode::AppendChild(aOldChild, aReturn);           \
  }                                                                         \
  NS_IMETHOD ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild,     \
                             nsIDOMNode** aReturn) {                        \
    return nsGenericDOMDataNode::ReplaceChild(aNewChild, aOldChild,         \
                                              aReturn);                     \
  }                                                                         \
  NS_IMETHOD RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn) {     \
    return nsGenericDOMDataNode::RemoveChild(aOldChild, aReturn);           \
  }                                                                         \
  NS_IMETHOD GetOwnerDocument(nsIDOMDocument** aOwnerDocument) {            \
    return nsGenericDOMDataNode::GetOwnerDocument(aOwnerDocument);          \
  }                                                                         \
  NS_IMETHOD GetNamespaceURI(nsAString& aNamespaceURI) {                    \
    return nsGenericDOMDataNode::GetNamespaceURI(aNamespaceURI);            \
  }                                                                         \
  NS_IMETHOD GetPrefix(nsAString& aPrefix) {                                \
    return nsGenericDOMDataNode::GetPrefix(aPrefix);                        \
  }                                                                         \
  NS_IMETHOD SetPrefix(const nsAString& aPrefix) {                          \
    return nsGenericDOMDataNode::SetPrefix(aPrefix);                        \
  }                                                                         \
  NS_IMETHOD Normalize() {                                                  \
    return NS_OK;                                                           \
  }                                                                         \
  NS_IMETHOD IsSupported(const nsAString& aFeature,                         \
                      const nsAString& aVersion,                            \
                      PRBool* aReturn) {                                    \
    return nsGenericDOMDataNode::IsSupported(aFeature, aVersion, aReturn);  \
  }                                                                         \
  NS_IMETHOD CloneNode(PRBool aDeep, nsIDOMNode** aReturn);

#endif /* nsGenericDOMDataNode_h___ */
