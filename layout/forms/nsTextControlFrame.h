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
 * The Original Code is mozilla.org code.
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

#ifndef nsTextControlFrame_h___
#define nsTextControlFrame_h___

#include "nsStackFrame.h"
#include "nsAreaFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIDOMMouseListener.h"
#include "nsIAnonymousContentCreator.h"
#include "nsIEditor.h"
#include "nsITextControlFrame.h"
#include "nsFormControlHelper.h"//for the inputdimensions
#include "nsIFontMetrics.h"
#include "nsWeakReference.h" //for service and presshell pointers
#include "nsIScrollableViewProvider.h"
#include "nsIPhonetic.h"

class nsISupportsArray;
class nsIEditor;
class nsISelectionController;
class nsTextInputSelectionImpl;
class nsTextInputListener;
class nsIDOMCharacterData;
class nsIScrollableView;
#ifdef ACCESSIBILITY
class nsIAccessible;
#endif


class nsTextControlFrame : public nsStackFrame,
                           public nsIAnonymousContentCreator,
                           public nsITextControlFrame,
                           public nsIScrollableViewProvider,
                           public nsIPhonetic

{
public:
  nsTextControlFrame(nsIPresShell* aShell);
  virtual ~nsTextControlFrame();

  virtual void RemovedAsPrimaryFrame(nsPresContext* aPresContext); 

  NS_IMETHOD Destroy(nsPresContext* aPresContext);

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD  Paint(nsPresContext*      aPresContext,
                    nsIRenderingContext& aRenderingContext,
                    const nsRect&        aDirtyRect,
                    nsFramePaintLayer    aWhichLayer,
                    PRUint32             aFlags = 0);

  NS_IMETHOD GetFrameForPoint(const nsPoint& aPoint,
                              nsFramePaintLayer aWhichLayer,
                              nsIFrame** aFrame);

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext,
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

  NS_IMETHOD GetPrefSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetMinSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetMaxSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetAscent(nsBoxLayoutState& aBoxLayoutState, nscoord& aAscent);

  virtual PRBool IsLeaf() const;
  
#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#endif

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    aResult.AssignLiteral("nsTextControlFrame");
    return NS_OK;
  }
#endif

  // from nsIAnonymousContentCreator
  NS_IMETHOD CreateAnonymousContent(nsPresContext* aPresContext,
                                    nsISupportsArray& aChildList);
  NS_IMETHOD CreateFrameFor(nsPresContext*   aPresContext,
                               nsIContent *      aContent,
                               nsIFrame**        aFrame);
  virtual void PostCreateFrames();

  // Utility methods to set current widget state

  // Be careful when using this method.
  // Calling it may cause |this| to be deleted.
  // In that case the method returns an error value.
  nsresult SetValue(const nsAString& aValue);
  NS_IMETHOD SetInitialChildList(nsPresContext* aPresContext,
                                  nsIAtom*        aListName,
                                  nsIFrame*       aChildList);

//==== BEGIN NSIFORMCONTROLFRAME
  NS_IMETHOD_(PRInt32) GetFormControlType() const; //*
  NS_IMETHOD GetName(nsAString* aName);//*
  virtual void SetFocus(PRBool aOn , PRBool aRepaint); 
  virtual void ScrollIntoView(nsPresContext* aPresContext);
  virtual nscoord GetVerticalInsidePadding(nsPresContext* aPresContext,
                                           float aPixToTwip,
                                           nscoord aInnerHeight) const;
  virtual nscoord GetHorizontalInsidePadding(nsPresContext* aPresContext,
                                             float aPixToTwip, 
                                             nscoord aInnerWidth,
                                             nscoord aCharWidth) const;/**/
  NS_IMETHOD SetSuggestedSize(nscoord aWidth, nscoord aHeight);
  NS_IMETHOD GetFormContent(nsIContent*& aContent) const;
  NS_IMETHOD SetProperty(nsPresContext* aPresContext, nsIAtom* aName, const nsAString& aValue);
  NS_IMETHOD GetProperty(nsIAtom* aName, nsAString& aValue); 
  NS_IMETHOD OnContentReset();


//==== END NSIFORMCONTROLFRAME

//==== NSIGFXTEXTCONTROLFRAME2

  NS_IMETHOD    GetEditor(nsIEditor **aEditor);
  NS_IMETHOD    OwnsValue(PRBool* aOwnsValue);
  NS_IMETHOD    GetValue(nsAString& aValue, PRBool aIgnoreWrap);
  NS_IMETHOD    GetTextLength(PRInt32* aTextLength);
  NS_IMETHOD    CheckFireOnChange();
  NS_IMETHOD    SetSelectionStart(PRInt32 aSelectionStart);
  NS_IMETHOD    SetSelectionEnd(PRInt32 aSelectionEnd);
  NS_IMETHOD    SetSelectionRange(PRInt32 aSelectionStart, PRInt32 aSelectionEnd);
  NS_IMETHOD    GetSelectionRange(PRInt32* aSelectionStart, PRInt32* aSelectionEnd);
  NS_IMETHOD    GetSelectionContr(nsISelectionController **aSelCon);

  // nsIPhonetic
  NS_DECL_NSIPHONETIC

//==== END NSIGFXTEXTCONTROLFRAME2
//==== OVERLOAD of nsIFrame
  virtual nsIAtom* GetType() const;

  /** handler for attribute changes to mContent */
  NS_IMETHOD AttributeChanged(nsIContent*     aChild,
                              PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

  NS_IMETHOD GetText(nsString* aText);

  NS_DECL_ISUPPORTS_INHERITED

public: //for methods who access nsTextControlFrame directly

  void FireOnInput();//notify that we have some kind of change.
  /**
   * Find out whether this is a single line text control.  (text or password)
   * @return whether this is a single line text control
   */
  PRBool IsSingleLineTextControl() const;
  /**
   * Find out whether this control is a textarea.
   * @return whether this is a textarea text control
   */
  PRBool IsTextArea() const;
  /**
   * Find out whether this control edits plain text.  (Currently always true.)
   * @return whether this is a plain text control
   */
  PRBool IsPlainTextControl() const;
  /**
   * Find out whether this is a password control (input type=password)
   * @return whether this is a password ontrol
   */
  PRBool IsPasswordTextControl() const;
  void SetValueChanged(PRBool aValueChanged);
  /** Called when the frame is focused, to remember the value for onChange. */
  nsresult InitFocusedValue();
  nsresult DOMPointToOffset(nsIDOMNode* aNode, PRInt32 aNodeOffset, PRInt32 *aResult);
  nsresult OffsetToDOMPoint(PRInt32 aOffset, nsIDOMNode** aResult, PRInt32* aPosition);

  /* called to free up native keybinding services */
  static NS_HIDDEN_(void) ShutDown();

protected:

  /**
   * Find out whether this control is scrollable (i.e. if it is not a single
   * line text control)
   * @return whether this control is scrollable
   */
  PRBool IsScrollable() const;
  /**
   * Initialize mEditor with the proper flags and the default value.
   * @throws NS_ERROR_NOT_INITIALIZED if mEditor has not been created
   * @throws various and sundry other things
   */
  nsresult InitEditor();
  /**
   * Strip all \n, \r and nulls from the given string
   * @param aString the string to remove newlines from [in/out]
   */
  void RemoveNewlines(nsString &aString);
  /**
   * Get the maxlength attribute
   * @param aMaxLength the value of the max length attr
   * @throws NS_CONTENT_ATTR_NOT_THERE if attr not defined
   */
  nsresult GetMaxLength(PRInt32* aMaxLength);
  /**
   * Find out whether an attribute exists on the content or not.
   * @param aAtt the attribute to determine the existence of
   * @returns PR_FALSE if it does not exist
   */
  PRBool AttributeExists(nsIAtom *aAtt) const
  { return mContent && mContent->HasAttr(kNameSpaceID_None, aAtt); }

  /**
   * We call this when we are being destroyed or removed from the PFM.
   * @param aPresContext the current pres context
   */
  void PreDestroy(nsPresContext* aPresContext);
  /**
   * Fire the onChange event.
   */
  nsresult FireOnChange();

//helper methods
  nsresult GetSizeFromContent(PRInt32* aSize) const;

  /**
   * Get the cols attribute (if textarea) or a default
   * @return the number of columns to use
   */
  PRInt32 GetCols();
  /**
   * Get the rows attribute (if textarea) or a default
   * @return the number of rows to use
   */
  PRInt32 GetRows();

  nsresult ReflowStandard(nsPresContext*          aPresContext,
                          nsSize&                  aDesiredSize,
                          const nsHTMLReflowState& aReflowState,
                          nsReflowStatus&          aStatus);

  nsresult CalculateSizeStandard(nsPresContext*       aPresContext,
                                 const nsHTMLReflowState& aReflowState,
                                 nsSize&               aDesiredSize,
                                 nsSize&               aMinSize);

  PRInt32 GetWidthInCharacters() const;

  // nsIScrollableViewProvider
  virtual nsIScrollableView* GetScrollableView();

private:
  //helper methods
  nsresult SetSelectionInternal(nsIDOMNode *aStartNode, PRInt32 aStartOffset,
                                nsIDOMNode *aEndNode, PRInt32 aEndOffset);
  nsresult SelectAllContents();
  nsresult SetSelectionEndPoints(PRInt32 aSelStart, PRInt32 aSelEnd);
  
private:
  nsCOMPtr<nsIEditor> mEditor;
  nsCOMPtr<nsISelectionController> mSelCon;

  //cached sizes and states
  nscoord      mSuggestedWidth;
  nscoord      mSuggestedHeight;
  nsSize       mSize;

  // these packed bools could instead use the high order bits on mState, saving 4 bytes 
  PRPackedBool mUseEditor;
  PRPackedBool mIsProcessing;
  PRPackedBool mNotifyOnInput;//default this to off to stop any notifications until setup is complete
  PRPackedBool mDidPreDestroy; // has PreDestroy been called        

  nsTextInputSelectionImpl *mTextSelImpl;
  nsTextInputListener *mTextListener;
  nsIScrollableView *mScrollableView;
  nsString mFocusedValue;
};

#endif


