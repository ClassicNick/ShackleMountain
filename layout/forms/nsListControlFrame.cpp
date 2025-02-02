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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   Mats Palmgren <mats.palmgren@bredband.net>
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

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsListControlFrame.h"
#include "nsFormControlFrame.h" // for COMPARE macro
#include "nsFormControlHelper.h"
#include "nsHTMLAtoms.h"
#include "nsIFormControl.h"
#include "nsIDeviceContext.h" 
#include "nsIDocument.h"
#include "nsIDOMHTMLCollection.h" 
#include "nsIDOMHTMLOptionsCollection.h" 
#include "nsIDOMNSHTMLOptionCollectn.h"
#include "nsIDOMHTMLSelectElement.h" 
#include "nsIDOMNSHTMLSelectElement.h" 
#include "nsIDOMHTMLOptionElement.h" 
#include "nsComboboxControlFrame.h"
#include "nsIViewManager.h"
#include "nsIScrollableView.h"
#include "nsIDOMHTMLOptGroupElement.h"
#include "nsWidgetsCID.h"
#include "nsHTMLReflowCommand.h"
#include "nsIPresShell.h"
#include "nsHTMLParts.h"
#include "nsIDOMEventReceiver.h"
#include "nsIEventStateManager.h"
#include "nsIEventListenerManager.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIComponentManager.h"
#include "nsILookAndFeel.h"
#include "nsLayoutAtoms.h"
#include "nsIFontMetrics.h"
#include "nsVoidArray.h"
#include "nsIScrollableFrame.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMNSEvent.h"
#include "nsGUIEvent.h"
#include "nsIServiceManager.h"
#include "nsINodeInfo.h"
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#endif
#include "nsISelectElement.h"
#include "nsIPrivateDOMEvent.h"
#include "nsCSSRendering.h"
#include "nsReflowPath.h"
#include "nsITheme.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMMouseMotionListener.h"
#include "nsIDOMKeyListener.h"
#include "nsLayoutUtils.h"

// Constants
const nscoord kMaxDropDownRows          = 20; // This matches the setting for 4.x browsers
const PRInt32 kDefaultMultiselectHeight = 4; // This is compatible with 4.x browsers
const PRInt32 kNothingSelected          = -1;
const PRInt32 kMaxZ                     = 0x7fffffff; //XXX: Shouldn't there be a define somewhere for MaxInt for PRInt32
const PRInt32 kNoSizeSpecified          = -1;


nsListControlFrame * nsListControlFrame::mFocused = nsnull;

// Using for incremental typing navigation
#define INCREMENTAL_SEARCH_KEYPRESS_TIME 1000
// XXX, kyle.yuan@sun.com, there are 4 definitions for the same purpose:
//  nsMenuPopupFrame.h, nsListControlFrame.cpp, listbox.xml, tree.xml
//  need to find a good place to put them together.
//  if someone changes one, please also change the other.

DOMTimeStamp nsListControlFrame::gLastKeyTime = 0;

/******************************************************************************
 * nsListEventListener
 * This class is responsible for propagating events to the nsListControlFrame.
 * Frames are not refcounted so they can't be used as event listeners.
 *****************************************************************************/

class nsListEventListener : public nsIDOMKeyListener,
                            public nsIDOMMouseListener,
                            public nsIDOMMouseMotionListener
{
public:
  nsListEventListener(nsListControlFrame *aFrame)
    : mFrame(aFrame) { }

  void SetFrame(nsListControlFrame *aFrame) { mFrame = aFrame; }

  NS_DECL_ISUPPORTS

  // nsIDOMEventListener
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

  // nsIDOMKeyListener
  NS_IMETHOD KeyDown(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyUp(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyPress(nsIDOMEvent* aKeyEvent);

  // nsIDOMMouseListener
  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent);

  // nsIDOMMouseMotionListener
  NS_IMETHOD MouseMove(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD DragMove(nsIDOMEvent* aMouseEvent);

private:
  nsListControlFrame  *mFrame;
};

//---------------------------------------------------------
nsresult
NS_NewListControlFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsListControlFrame* it =
    new (aPresShell) nsListControlFrame(aPresShell, aPresShell->GetDocument());
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  it->AddStateBits(NS_FRAME_INDEPENDENT_SELECTION);
#if 0
  // set the state flags (if any are provided)
  it->AddStateBits(NS_BLOCK_SPACE_MGR);
#endif
  *aNewFrame = it;
  return NS_OK;
}

//-----------------------------------------------------------
// Reflow Debugging Macros
// These let us "see" how many reflow counts are happening
//-----------------------------------------------------------
#ifdef DO_REFLOW_COUNTER

#define MAX_REFLOW_CNT 1024
static PRInt32 gTotalReqs    = 0;;
static PRInt32 gTotalReflows = 0;;
static PRInt32 gReflowControlCntRQ[MAX_REFLOW_CNT];
static PRInt32 gReflowControlCnt[MAX_REFLOW_CNT];
static PRInt32 gReflowInx = -1;

#define REFLOW_COUNTER() \
  if (mReflowId > -1) \
    gReflowControlCnt[mReflowId]++;

#define REFLOW_COUNTER_REQUEST() \
  if (mReflowId > -1) \
    gReflowControlCntRQ[mReflowId]++;

#define REFLOW_COUNTER_DUMP(__desc) \
  if (mReflowId > -1) {\
    gTotalReqs    += gReflowControlCntRQ[mReflowId];\
    gTotalReflows += gReflowControlCnt[mReflowId];\
    printf("** Id:%5d %s RF: %d RQ: %d   %d/%d  %5.2f\n", \
           mReflowId, (__desc), \
           gReflowControlCnt[mReflowId], \
           gReflowControlCntRQ[mReflowId],\
           gTotalReflows, gTotalReqs, float(gTotalReflows)/float(gTotalReqs)*100.0f);\
  }

#define REFLOW_COUNTER_INIT() \
  if (gReflowInx < MAX_REFLOW_CNT) { \
    gReflowInx++; \
    mReflowId = gReflowInx; \
    gReflowControlCnt[mReflowId] = 0; \
    gReflowControlCntRQ[mReflowId] = 0; \
  } else { \
    mReflowId = -1; \
  }

// reflow messages
#define REFLOW_DEBUG_MSG(_msg1) printf((_msg1))
#define REFLOW_DEBUG_MSG2(_msg1, _msg2) printf((_msg1), (_msg2))
#define REFLOW_DEBUG_MSG3(_msg1, _msg2, _msg3) printf((_msg1), (_msg2), (_msg3))
#define REFLOW_DEBUG_MSG4(_msg1, _msg2, _msg3, _msg4) printf((_msg1), (_msg2), (_msg3), (_msg4))

#else //-------------

#define REFLOW_COUNTER_REQUEST() 
#define REFLOW_COUNTER() 
#define REFLOW_COUNTER_DUMP(__desc) 
#define REFLOW_COUNTER_INIT() 

#define REFLOW_DEBUG_MSG(_msg) 
#define REFLOW_DEBUG_MSG2(_msg1, _msg2) 
#define REFLOW_DEBUG_MSG3(_msg1, _msg2, _msg3) 
#define REFLOW_DEBUG_MSG4(_msg1, _msg2, _msg3, _msg4) 


#endif

//------------------------------------------
// This is for being VERY noisy
//------------------------------------------
#ifdef DO_VERY_NOISY
#define REFLOW_NOISY_MSG(_msg1) printf((_msg1))
#define REFLOW_NOISY_MSG2(_msg1, _msg2) printf((_msg1), (_msg2))
#define REFLOW_NOISY_MSG3(_msg1, _msg2, _msg3) printf((_msg1), (_msg2), (_msg3))
#define REFLOW_NOISY_MSG4(_msg1, _msg2, _msg3, _msg4) printf((_msg1), (_msg2), (_msg3), (_msg4))
#else
#define REFLOW_NOISY_MSG(_msg) 
#define REFLOW_NOISY_MSG2(_msg1, _msg2) 
#define REFLOW_NOISY_MSG3(_msg1, _msg2, _msg3) 
#define REFLOW_NOISY_MSG4(_msg1, _msg2, _msg3, _msg4) 
#endif

//------------------------------------------
// Displays value in pixels or twips
//------------------------------------------
#ifdef DO_PIXELS
#define PX(__v) __v / 15
#else
#define PX(__v) __v 
#endif

//------------------------------------------
// Asserts if we return a desired size that 
// doesn't correctly match the mComputedWidth
//------------------------------------------
#ifdef DO_UNCONSTRAINED_CHECK
#define UNCONSTRAINED_CHECK() \
if (aReflowState.mComputedWidth != NS_UNCONSTRAINEDSIZE) { \
  nscoord width = aDesiredSize.width - borderPadding.left - borderPadding.right; \
  if (width != aReflowState.mComputedWidth) { \
    printf("aDesiredSize.width %d %d != aReflowState.mComputedWidth %d\n", aDesiredSize.width, width, aReflowState.mComputedWidth); \
  } \
  NS_ASSERTION(width == aReflowState.mComputedWidth, "Returning bad value when constrained!"); \
}
#else
#define UNCONSTRAINED_CHECK()
#endif
//------------------------------------------------------
//-- Done with macros
//------------------------------------------------------

//---------------------------------------------------------
nsListControlFrame::nsListControlFrame(nsIPresShell* aShell,
  nsIDocument* aDocument)
  : nsHTMLScrollFrame(aShell, PR_FALSE)
{
  mComboboxFrame      = nsnull;
  mChangesSinceDragStart = PR_FALSE;
  mButtonDown         = PR_FALSE;
  mMaxWidth           = 0;
  mMaxHeight          = 0;

  mIsAllContentHere   = PR_FALSE;
  mIsAllFramesHere    = PR_FALSE;
  mHasBeenInitialized = PR_FALSE;
  mNeedToReset        = PR_TRUE;
  mPostChildrenLoadedReset = PR_FALSE;

  mCacheSize.width             = -1;
  mCacheSize.height            = -1;
  mCachedMaxElementWidth       = -1;
  mCachedAvailableSize.width   = -1;
  mCachedAvailableSize.height  = -1;
  mCachedUnconstrainedSize.width  = -1;
  mCachedUnconstrainedSize.height = -1;
  
  mOverrideReflowOpt           = PR_FALSE;
  mPassId                      = 0;

  mDummyFrame                  = nsnull;

  mControlSelectMode           = PR_FALSE;
  REFLOW_COUNTER_INIT()
}

//---------------------------------------------------------
nsListControlFrame::~nsListControlFrame()
{
  REFLOW_COUNTER_DUMP("nsLCF");

  mComboboxFrame = nsnull;
}

// for Bug 47302 (remove this comment later)
NS_IMETHODIMP
nsListControlFrame::Destroy(nsPresContext *aPresContext)
{
  // get the receiver interface from the browser button's content node
  nsCOMPtr<nsIDOMEventReceiver> receiver(do_QueryInterface(mContent));

  // Clear the frame pointer on our event listener, just in case the
  // event listener can outlive the frame.

  mEventListener->SetFrame(nsnull);

  receiver->RemoveEventListenerByIID(NS_STATIC_CAST(nsIDOMMouseListener*,
                                                    mEventListener),
                                     NS_GET_IID(nsIDOMMouseListener));

  receiver->RemoveEventListenerByIID(NS_STATIC_CAST(nsIDOMMouseMotionListener*,
                                                    mEventListener),
                                     NS_GET_IID(nsIDOMMouseMotionListener));

  receiver->RemoveEventListenerByIID(NS_STATIC_CAST(nsIDOMKeyListener*,
                                                    mEventListener),
                                     NS_GET_IID(nsIDOMKeyListener));

  nsFormControlFrame::RegUnRegAccessKey(aPresContext, NS_STATIC_CAST(nsIFrame*, this), PR_FALSE);
  return nsHTMLScrollFrame::Destroy(aPresContext);
}

NS_IMETHODIMP
nsListControlFrame::Paint(nsPresContext*      aPresContext,
                          nsIRenderingContext& aRenderingContext,
                          const nsRect&        aDirtyRect,
                          nsFramePaintLayer    aWhichLayer,
                          PRUint32             aFlags)
{
  if (!GetStyleVisibility()->IsVisible()) {
    return PR_FALSE;
  }

  // Don't allow painting of list controls when painting is suppressed.
  PRBool paintingSuppressed = PR_FALSE;
  aPresContext->PresShell()->IsPaintingSuppressed(&paintingSuppressed);
  if (paintingSuppressed)
    return NS_OK;

  // Start by assuming we are visible and need to be painted
  PRBool isVisible = PR_TRUE;

  if (aPresContext->IsPaginated()) {
    if (aPresContext->IsRenderingOnlySelection()) {
      // Check the quick way first
      PRBool isSelected = (mState & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT;
      // if we aren't selected in the mState we could be a container
      // so check to see if we are in the selection range
      if (!isSelected) {
        nsCOMPtr<nsISelectionController> selcon;
        selcon = do_QueryInterface(aPresContext->PresShell());
        if (selcon) {
          nsCOMPtr<nsISelection> selection;
          selcon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));
          nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mContent));
          selection->ContainsNode(node, PR_TRUE, &isVisible);
        } else {
          isVisible = PR_FALSE;
        }
      }
    }
  } 

  if (isVisible) {
    if (aWhichLayer == NS_FRAME_PAINT_LAYER_BACKGROUND) {
      const nsStyleDisplay* displayData = GetStyleDisplay();
      if (displayData->mAppearance) {
        nsITheme *theme = aPresContext->GetTheme();
        nsRect  rect(0, 0, mRect.width, mRect.height);
        if (theme && theme->ThemeSupportsWidget(aPresContext, this, displayData->mAppearance))
          theme->DrawWidgetBackground(&aRenderingContext, this, 
                                      displayData->mAppearance, rect, aDirtyRect); 
      }
    }

    return nsHTMLScrollFrame::Paint(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);
  }

  DO_GLOBAL_REFLOW_COUNT_DSP("nsListControlFrame", &aRenderingContext);
  return NS_OK;

}

/* Note: this is called by the SelectsAreaFrame, which is the same
   as the frame returned by GetOptionsContainer. It's the frame which is
   scrolled by us. */
void nsListControlFrame::PaintFocus(nsIRenderingContext& aRC, nsFramePaintLayer aWhichLayer)
{
  if (NS_FRAME_PAINT_LAYER_FOREGROUND != aWhichLayer) return;

  if (mFocused != this) return;

  // The mEndSelectionIndex is what is currently being selected
  // use the selected index if this is kNothingSelected
  PRInt32 focusedIndex;
  if (mEndSelectionIndex == kNothingSelected) {
    GetSelectedIndex(&focusedIndex);
  } else {
    focusedIndex = mEndSelectionIndex;
  }

  nsPresContext* presContext = GetPresContext();
  if (!GetScrollableView()) return;

  nsIPresShell *presShell = presContext->GetPresShell();
  if (!presShell) return;

  nsIFrame* containerFrame;
  GetOptionsContainer(presContext, &containerFrame);
  if (!containerFrame) return;

  nsIFrame * childframe = nsnull;
  nsresult result = NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> focusedContent;

  nsCOMPtr<nsIDOMNSHTMLSelectElement> selectNSElement(do_QueryInterface(mContent));
  NS_ASSERTION(selectNSElement, "Can't be null");

  nsCOMPtr<nsISelectElement> selectElement(do_QueryInterface(mContent));
  NS_ASSERTION(selectElement, "Can't be null");

  // If we have a selected index then get that child frame
  if (focusedIndex != kNothingSelected) {
    focusedContent = GetOptionContent(focusedIndex);
    // otherwise we find the focusedContent's frame and scroll to it
    if (focusedContent) {
      result = presShell->GetPrimaryFrameFor(focusedContent, &childframe);
    }
  } else {
    nsCOMPtr<nsIDOMHTMLSelectElement> selectHTMLElement(do_QueryInterface(mContent));
    NS_ASSERTION(selectElement, "Can't be null");

    // Since there isn't a selected item we need to show a focus ring around the first
    // non-disabled item and skip all the option group elements (nodes)
    nsCOMPtr<nsIDOMNode> node;

    PRUint32 length;
    selectHTMLElement->GetLength(&length);
    if (length) {
      // find the first non-disabled item
      PRBool isDisabled = PR_TRUE;
      for (PRInt32 i=0;i<PRInt32(length) && isDisabled;i++) {
        if (NS_FAILED(selectNSElement->Item(i, getter_AddRefs(node))) || !node) {
          break;
        }
        if (NS_FAILED(selectElement->IsOptionDisabled(i, &isDisabled))) {
          break;
        }
        if (isDisabled) {
          node = nsnull;
        } else {
          break;
        }
      }
      if (!node) {
        return;
      }
    }

    // if we found a node use, if not get the first child (this is for empty selects)
    if (node) {
      focusedContent = do_QueryInterface(node);
      result = presShell->GetPrimaryFrameFor(focusedContent, &childframe);
    }
    if (!childframe) {
      // The only way we can get right here is that there are no options
      // and we need to get the dummy frame so it has the focus ring
      childframe = containerFrame->GetFirstChild(nsnull);
      result = NS_OK;
    }
  }

  if (NS_FAILED(result) || !childframe) return;

  // get the child rect
  nsRect fRect = childframe->GetRect();
  
  // get it into the coordinates of containerFrame
  fRect.MoveBy(childframe->GetParent()->GetOffsetTo(containerFrame));
  PRBool lastItemIsSelected = PR_FALSE;
  if (focusedIndex != kNothingSelected) {
    nsCOMPtr<nsIDOMNode> node;
    if (NS_SUCCEEDED(selectNSElement->Item(focusedIndex, getter_AddRefs(node)))) {
      nsCOMPtr<nsIDOMHTMLOptionElement> domOpt(do_QueryInterface(node));
      NS_ASSERTION(domOpt, "Something has gone seriously awry.  This should be an option element!");
      domOpt->GetSelected(&lastItemIsSelected);
    }
  }

  // set up back stop colors and then ask L&F service for the real colors
  nscolor color;
  presContext->LookAndFeel()->
    GetColor(lastItemIsSelected ?
             nsILookAndFeel::eColor_WidgetSelectForeground :
             nsILookAndFeel::eColor_WidgetSelectBackground, color);

  nscoord onePixelInTwips = presContext->IntScaledPixelsToTwips(1);

  nsRect dirty;
  nscolor colors[] = {color, color, color, color};
  PRUint8 borderStyle[] = {NS_STYLE_BORDER_STYLE_DOTTED, NS_STYLE_BORDER_STYLE_DOTTED, NS_STYLE_BORDER_STYLE_DOTTED, NS_STYLE_BORDER_STYLE_DOTTED};
  nsRect innerRect = fRect;
  innerRect.Deflate(nsSize(onePixelInTwips, onePixelInTwips));
  nsCSSRendering::DrawDashedSides(0, aRC, dirty, borderStyle, colors, fRect, innerRect, 0, nsnull);

}

//---------------------------------------------------------
// Frames are not refcounted, no need to AddRef
NS_IMETHODIMP
nsListControlFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(NS_GET_IID(nsIFormControlFrame))) {
    *aInstancePtr = (void*) ((nsIFormControlFrame*) this);
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIListControlFrame))) {
    *aInstancePtr = (void *)((nsIListControlFrame*)this);
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsISelectControlFrame))) {
    *aInstancePtr = (void *)((nsISelectControlFrame*)this);
    return NS_OK;
  }
  return nsHTMLScrollFrame::QueryInterface(aIID, aInstancePtr);
}

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsListControlFrame::GetAccessible(nsIAccessible** aAccessible)
{
  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");

  if (accService) {
    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(mContent);
    return accService->CreateHTMLListboxAccessible(node, GetPresContext(),
                                                   aAccessible);
  }

  return NS_ERROR_FAILURE;
}
#endif

//---------------------------------------------------------
// Reflow is overriden to constrain the listbox height to the number of rows and columns
// specified. 
#ifdef DO_REFLOW_DEBUG
static int myCounter = 0;

static void printSize(char * aDesc, nscoord aSize) 
{
  printf(" %s: ", aDesc);
  if (aSize == NS_UNCONSTRAINEDSIZE) {
    printf("UNC");
  } else {
    printf("%d", aSize);
  }
}
#endif

static nscoord
GetMaxOptionHeight(nsIFrame* aContainer)
{
  nscoord result = 0;
  for (nsIFrame* option = aContainer->GetFirstChild(nsnull);
       option; option = option->GetNextSibling()) {
    nscoord optionHeight;
    if (nsCOMPtr<nsIDOMHTMLOptGroupElement>
        (do_QueryInterface(option->GetContent()))) {
      // an optgroup
      optionHeight = GetMaxOptionHeight(option);
    } else {
      // an option
      optionHeight = option->GetSize().height;
    }
    if (result < optionHeight)
      result = optionHeight;
  }
  return result;
}

static inline PRBool
IsOptGroup(nsIContent *aContent)
{
  nsINodeInfo *ni = aContent->GetNodeInfo();
  return (ni && ni->Equals(nsHTMLAtoms::optgroup) &&
          aContent->IsContentOfType(nsIContent::eHTML));
}

static inline PRBool
IsOption(nsIContent *aContent)
{
  nsINodeInfo *ni = aContent->GetNodeInfo();
  return (ni && ni->Equals(nsHTMLAtoms::option) &&
          aContent->IsContentOfType(nsIContent::eHTML));
}

static PRUint32
GetNumberOfOptionsRecursive(nsIContent* aContent)
{
  PRUint32 optionCount = 0;
  const PRUint32 childCount = aContent ? aContent->GetChildCount() : 0;
  for (PRUint32 index = 0; index < childCount; ++index) {
    nsIContent* child = aContent->GetChildAt(index);
    if (::IsOption(child)) {
      ++optionCount;
    }
    else if (::IsOptGroup(child)) {
      optionCount += ::GetNumberOfOptionsRecursive(child);
    }
  }
  return optionCount;
}

static nscoord
GetOptGroupLabelsHeight(nsPresContext* aPresContext,
                        nsIContent*    aContent,
                        nscoord        aRowHeight)
{
  nscoord height = 0;
  const PRUint32 childCount = aContent ? aContent->GetChildCount() : 0;
  for (PRUint32 index = 0; index < childCount; ++index) {
    nsIContent* child = aContent->GetChildAt(index);
    if (::IsOptGroup(child)) {
      PRUint32 numOptions = ::GetNumberOfOptionsRecursive(child);
      nscoord optionsHeight = aRowHeight * numOptions;
      nsIFrame* frame = nsnull;
      aPresContext->GetPresShell()->GetPrimaryFrameFor(child, &frame);
      nscoord totalHeight = frame ? frame->GetSize().height : 0;
      height += PR_MAX(0, totalHeight - optionsHeight);
    }
  }
  return height;
}

//-----------------------------------------------------------------
// Main Reflow for ListBox/Dropdown
//-----------------------------------------------------------------
NS_IMETHODIMP 
nsListControlFrame::Reflow(nsPresContext*          aPresContext, 
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState, 
                           nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsListControlFrame", aReflowState.reason);
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  REFLOW_COUNTER_REQUEST();

  aStatus = NS_FRAME_COMPLETE;

#ifdef DO_REFLOW_DEBUG
  printf("%p ** Id: %d nsLCF::Reflow %d R: ", this, mReflowId, myCounter++);
  switch (aReflowState.reason) {
    case eReflowReason_Initial:
      printf("Initia");break;
    case eReflowReason_Incremental:
      printf("Increm");break;
    case eReflowReason_Resize:
      printf("Resize");break;
    case eReflowReason_StyleChange:
      printf("StyleC");break;
    case eReflowReason_Dirty:
      printf("Dirty ");break;
    default:printf("<unknown>%d", aReflowState.reason);break;
  }
  
  printSize("AW", aReflowState.availableWidth);
  printSize("AH", aReflowState.availableHeight);
  printSize("CW", aReflowState.mComputedWidth);
  printSize("CH", aReflowState.mComputedHeight);
  printf("\n");
#if 0
    {
      const nsStyleDisplay* display = GetStyleDisplay();
      printf("+++++++++++++++++++++++++++++++++ ");
      switch (display->mVisible) {
        case NS_STYLE_VISIBILITY_COLLAPSE: printf("NS_STYLE_VISIBILITY_COLLAPSE\n");break;
        case NS_STYLE_VISIBILITY_HIDDEN:   printf("NS_STYLE_VISIBILITY_HIDDEN\n");break;
        case NS_STYLE_VISIBILITY_VISIBLE:  printf("NS_STYLE_VISIBILITY_VISIBLE\n");break;
      }
    }
#endif
#endif // DEBUG_rodsXXX

  PRBool bailOnWidth;
  PRBool bailOnHeight;
  // This ifdef is for turning off the optimization
  // so we can check timings against the old version
#if 1

  nsFormControlFrame::SkipResizeReflow(mCacheSize,
                                       mCachedAscent,
                                       mCachedMaxElementWidth, 
                                       mCachedAvailableSize, 
                                       aDesiredSize, aReflowState, 
                                       aStatus, 
                                       bailOnWidth, bailOnHeight);

  // Here we bail if both the height and the width haven't changed
  // also we see if we should override the optimization
  //
  // The optimization can get overridden by the combobox 
  // sometime the combobox knows that the list MUST do a full reflow
  // no matter what
  if (!mOverrideReflowOpt && bailOnWidth && bailOnHeight) {
    REFLOW_DEBUG_MSG3("*** Done nsLCF - Bailing on DW: %d  DH: %d ", PX(aDesiredSize.width), PX(aDesiredSize.height));
    REFLOW_DEBUG_MSG3("bailOnWidth %d  bailOnHeight %d\n", PX(bailOnWidth), PX(bailOnHeight));
    NS_ASSERTION(aDesiredSize.width < 100000, "Width is still NS_UNCONSTRAINEDSIZE");
    NS_ASSERTION(aDesiredSize.height < 100000, "Height is still NS_UNCONSTRAINEDSIZE");
    return NS_OK;
  } else if (mOverrideReflowOpt) {
    mOverrideReflowOpt = PR_FALSE;
  }

#else
  bailOnWidth  = PR_FALSE;
  bailOnHeight = PR_FALSE;
#endif

#ifdef DEBUG_rodsXXX
  // Lists out all the options
  {
  nsresult rv = NS_ERROR_FAILURE; 
  nsCOMPtr<nsIDOMHTMLOptionsCollection> options =
    getter_AddRefs(GetOptions(mContent));
  if (options) {
    PRUint32 numOptions;
    options->GetLength(&numOptions);
    printf("--- Num of Items %d ---\n", numOptions);
    for (PRUint32 i=0;i<numOptions;i++) {
      nsCOMPtr<nsIDOMHTMLOptionElement> optionElement = getter_AddRefs(GetOption(options, i));
      if (optionElement) {
        nsAutoString text;
        rv = optionElement->GetLabel(text);
        if (NS_CONTENT_ATTR_HAS_VALUE != rv || text.IsEmpty()) {
          if (NS_OK != optionElement->GetText(text)) {
            text = "No Value";
          }
        } else {
            text = "No Value";
        }          
        printf("[%d] - %s\n", i, NS_LossyConvertUCS2toASCII(text).get());
      }
    }
  }
  }
#endif // DEBUG_rodsXXX


  // If all the content and frames are here 
  // then initialize it before reflow
    if (mIsAllContentHere && !mHasBeenInitialized) {
      if (PR_FALSE == mIsAllFramesHere) {
        CheckIfAllFramesHere();
      }
      if (mIsAllFramesHere && !mHasBeenInitialized) {
        mHasBeenInitialized = PR_TRUE;
      }
    }


    if (eReflowReason_Incremental == aReflowState.reason) {
      nsHTMLReflowCommand *command = aReflowState.path->mReflowCommand;
      if (command) {
        // XXX So this may do it too often
        // the side effect of this is if the user has scrolled to some other place in the list and
        // an incremental reflow comes through the list gets scrolled to the first selected item
        // I haven't been able to make it do it, but it will do it
        // basically the real solution is to know when all the reframes are there.
        PRInt32 selectedIndex = mEndSelectionIndex;
        if (selectedIndex == kNothingSelected) {
          GetSelectedIndex(&selectedIndex);
        }
        ScrollToIndex(selectedIndex);
      }
    }

   // Strategy: Let the inherited reflow happen as though the width and height of the
   // ScrollFrame are big enough to allow the listbox to
   // shrink to fit the longest option element line in the list.
   // The desired width and height returned by the inherited reflow is returned, 
   // unless one of the following has been specified.
   // 1. A css width has been specified.
   // 2. The size has been specified.
   // 3. The css height has been specified, but the number of rows has not.
   //  The size attribute overrides the height setting but the height setting
   // should be honored if there isn't a size specified.

    // Determine the desired width + height for the listbox + 
  aDesiredSize.width  = 0;
  aDesiredSize.height = 0;

  // Add the list frame as a child of the form
  if (eReflowReason_Initial == aReflowState.reason) {
    nsFormControlFrame::RegUnRegAccessKey(aPresContext, NS_STATIC_CAST(nsIFrame*, this), PR_TRUE);
  }

  //--Calculate a width just big enough for the scrollframe to shrink around the
  //longest element in the list
  nsHTMLReflowState secondPassState(aReflowState);
  nsHTMLReflowState firstPassState(aReflowState);

  //nsHTMLReflowState   firstPassState(aPresContext, nsnull,
  //                                   this, aDesiredSize);

   // Get the size of option elements inside the listbox
   // Compute the width based on the longest line in the listbox.
  
  firstPassState.mComputedWidth  = NS_UNCONSTRAINEDSIZE;
  firstPassState.mComputedHeight = NS_UNCONSTRAINEDSIZE;
  firstPassState.availableWidth  = NS_UNCONSTRAINEDSIZE;
  firstPassState.availableHeight = NS_UNCONSTRAINEDSIZE;
 
  nsHTMLReflowMetrics  scrolledAreaDesiredSize(PR_TRUE);


  if (eReflowReason_Incremental == aReflowState.reason) {
    nsHTMLReflowCommand *command = firstPassState.path->mReflowCommand;
    if (command) {
      nsReflowType type;
      command->GetType(type);
      firstPassState.reason = eReflowReason_StyleChange;
      firstPassState.path = nsnull;
    } else {
      nsresult res = nsHTMLScrollFrame::Reflow(aPresContext, 
                                               scrolledAreaDesiredSize,
                                               aReflowState, 
                                               aStatus);
      if (NS_FAILED(res)) {
        NS_ASSERTION(aDesiredSize.width < 100000, "Width is still NS_UNCONSTRAINEDSIZE");
        NS_ASSERTION(aDesiredSize.height < 100000, "Height is still NS_UNCONSTRAINEDSIZE");
        return res;
      }

      firstPassState.reason = eReflowReason_StyleChange;
      firstPassState.path = nsnull;
    }
  }

  if (mPassId == 0 || mPassId == 1) {
    if (mPassId == 0) {
      // We will reflow again, so tell the scrollframe not to scroll-to-restored-position
      // yet
      SetSuppressScrollbarUpdate(PR_TRUE);
    }
    nsresult res = nsHTMLScrollFrame::Reflow(aPresContext, 
                                             scrolledAreaDesiredSize,
                                             firstPassState, 
                                             aStatus);
    if (mPassId == 0) {
      SetSuppressScrollbarUpdate(PR_FALSE);
    }
    
    if (NS_FAILED(res)) {
      NS_ASSERTION(aDesiredSize.width < 100000, "Width is still NS_UNCONSTRAINEDSIZE");
      NS_ASSERTION(aDesiredSize.height < 100000, "Height is still NS_UNCONSTRAINEDSIZE");
      return res;
    }
    mCachedUnconstrainedSize.width  = scrolledAreaDesiredSize.width;
    mCachedUnconstrainedSize.height = scrolledAreaDesiredSize.height;
    mCachedAscent                   = scrolledAreaDesiredSize.ascent;
    mCachedDesiredMEW               = scrolledAreaDesiredSize.mMaxElementWidth;
  } else {
    scrolledAreaDesiredSize.width  = mCachedUnconstrainedSize.width;
    scrolledAreaDesiredSize.height = mCachedUnconstrainedSize.height;
    scrolledAreaDesiredSize.mMaxElementWidth = mCachedDesiredMEW;
  }

  // Compute the bounding box of the contents of the list using the area 
  // calculated by the first reflow as a starting point.
  //
  // The nsHTMLScrollFrame::Reflow adds border and padding to the
  // maxElementWidth, so these need to be subtracted
  nscoord scrolledAreaWidth  = scrolledAreaDesiredSize.width -
    (aReflowState.mComputedBorderPadding.left + aReflowState.mComputedBorderPadding.right);
  nscoord scrolledAreaHeight = scrolledAreaDesiredSize.height -
    (aReflowState.mComputedBorderPadding.top + aReflowState.mComputedBorderPadding.bottom);

  // Set up max values
  mMaxWidth  = scrolledAreaWidth;

  // Now the scrolledAreaWidth and scrolledAreaHeight are exactly 
  // wide and high enough to enclose their contents
  PRBool isInDropDownMode = IsInDropDownMode();

  nscoord visibleWidth = 0;
  if (NS_UNCONSTRAINEDSIZE == aReflowState.mComputedWidth) {
    visibleWidth = scrolledAreaWidth;
  } else {
    visibleWidth = aReflowState.mComputedWidth;
  }

   // Determine if a scrollbar will be needed, If so we need to add
   // enough the width to allow for the scrollbar.
   // The scrollbar will be needed under two conditions:
   // (size * heightOfaRow) < scrolledAreaHeight or
   // the height set through Style < scrolledAreaHeight.

   // Calculate the height of a single row in the listbox or dropdown
   // list by using the tallest of the grandchildren, since there may be
   // option groups in addition to option elements, either of which may
   // be visible or invisible.
  nsIFrame *optionsContainer;
  GetOptionsContainer(aPresContext, &optionsContainer);
  PRInt32 heightOfARow = GetMaxOptionHeight(optionsContainer);

  // Check to see if we have zero items 
  PRInt32 length = 0;
  GetNumberOfOptions(&length);

  // If there is only one option and that option's content is empty
  // then heightOfARow is zero, so we need to go measure 
  // the height of the option as if it had some text.
  if (heightOfARow == 0 && length > 0) {
    nsCOMPtr<nsIContent> option = GetOptionContent(0);
    if (option) {
      nsIFrame * optFrame;
      nsresult result = GetPresContext()->PresShell()->
        GetPrimaryFrameFor(option, &optFrame);
      if (NS_SUCCEEDED(result) && optFrame != nsnull) {
        nsStyleContext* optStyle = optFrame->GetStyleContext();
        if (optStyle) {
          const nsStyleFont* styleFont = optStyle->GetStyleFont();
          nsCOMPtr<nsIFontMetrics> fontMet;
          result = aPresContext->DeviceContext()->
            GetMetricsFor(styleFont->mFont, *getter_AddRefs(fontMet));
          if (NS_SUCCEEDED(result) && fontMet) {
            if (fontMet) {
              fontMet->GetHeight(heightOfARow);
              mMaxHeight = heightOfARow;
            }
          }
        }
      }
    }
  }
  mMaxHeight = heightOfARow;

  // Check to see if we have no width and height
  // The following code measures the width and height 
  // of a bogus string so the list actually displays
  nscoord visibleHeight = 0;
  if (isInDropDownMode) {
    // Compute the visible height of the drop-down list
    // The dropdown list height is the smaller of it's height setting or the height
    // of the smallest box that can drawn around it's contents.
    visibleHeight = scrolledAreaHeight;

    mNumDisplayRows = kMaxDropDownRows;
    if (visibleHeight > (mNumDisplayRows * heightOfARow)) {
      visibleHeight = (mNumDisplayRows * heightOfARow);
      // This is an adaptive algorithm for figuring out how many rows 
      // should be displayed in the drop down. The standard size is 20 rows, 
      // but on 640x480 it is typically too big.
      // This takes the height of the screen divides it by two and then subtracts off 
      // an estimated height of the combobox. I estimate it by taking the max element size
      // of the drop down and multiplying it by 2 (this is arbitrary) then subtract off
      // the border and padding of the drop down (again rather arbitrary)
      // This all breaks down if the font of the combobox is a lot larger then the option items
      // or CSS style has set the height of the combobox to be rather large.
      // We can fix these cases later if they actually happen.
      if (isInDropDownMode) {
        nscoord screenHeightInPixels = 0;
        if (NS_SUCCEEDED(nsFormControlFrame::GetScreenHeight(aPresContext, screenHeightInPixels))) {
          float   p2t;
          p2t = aPresContext->PixelsToTwips();
          nscoord screenHeight = NSIntPixelsToTwips(screenHeightInPixels, p2t);

          nscoord availDropHgt = (screenHeight / 2) - (heightOfARow*2); // approx half screen minus combo size
          availDropHgt -= aReflowState.mComputedBorderPadding.top + aReflowState.mComputedBorderPadding.bottom;

          nscoord hgt = visibleHeight + aReflowState.mComputedBorderPadding.top + aReflowState.mComputedBorderPadding.bottom;
          if (heightOfARow > 0) {
            if (hgt > availDropHgt) {
              visibleHeight = (availDropHgt / heightOfARow) * heightOfARow;
            }
            mNumDisplayRows = visibleHeight / heightOfARow;
          } else {
            // Hmmm, not sure what to do here. Punt, and make both of them one
            visibleHeight   = 1;
            mNumDisplayRows = 1;
          }
        }
      }
    }
   
  } else {
      // Calculate the visible height of the listbox
    if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedHeight) {
      visibleHeight = aReflowState.mComputedHeight;
    } else {
      mNumDisplayRows = 1;
      GetSizeAttribute(&mNumDisplayRows);
      if (mNumDisplayRows >= 1) {
        visibleHeight = mNumDisplayRows * heightOfARow;
      } else {
        // When SIZE=0 or unspecified we constrain the height to [2..kMaxDropDownRows] rows.
        // We add in the height of optgroup labels (within the constraint above), bug 300474.
        visibleHeight = ::GetOptGroupLabelsHeight(GetPresContext(), mContent, heightOfARow);

        PRBool multipleSelections = PR_FALSE;
        GetMultiple(&multipleSelections);
        if (multipleSelections) {
          if (length < 2) {
            // Add in 1 heightOfARow also when length==0 to match how we calculate the desired size.
            visibleHeight = heightOfARow + PR_MAX(heightOfARow, visibleHeight);
          } else {
            visibleHeight = PR_MIN(kMaxDropDownRows * heightOfARow, length * heightOfARow + visibleHeight);
          }
        } else {
          visibleHeight += 2 * heightOfARow;
        }
      }
    }
  }

  // There are no items in the list
  // but we want to include space for the scrollbars
  // So fake like we will need scrollbars also
  if (!isInDropDownMode && 0 == length) {
    if (aReflowState.mComputedWidth != 0) {
      scrolledAreaHeight = visibleHeight+1;
    }
  }

  // The visible height is zero, this could be a select with no options
  // or a select with a single option that has no content or no label
  //
  // So this may not be the best solution, but we get the height of the font
  // for the list frame and use that as the max/minimum size for the contents
  if (visibleHeight == 0) {
    if (aReflowState.mComputedHeight != 0) {
      nsCOMPtr<nsIFontMetrics> fontMet;
      nsresult rvv = nsFormControlHelper::GetFrameFontFM(this, getter_AddRefs(fontMet));
      if (NS_SUCCEEDED(rvv) && fontMet) {
        aReflowState.rendContext->SetFont(fontMet);
        fontMet->GetHeight(visibleHeight);
        mMaxHeight = visibleHeight;
      }
    }
  }

  // When in dropdown mode make sure we obey min/max-width and min/max-height
  if (!isInDropDownMode) {
    nscoord fullWidth = visibleWidth + aReflowState.mComputedBorderPadding.left + aReflowState.mComputedBorderPadding.right;
    if (fullWidth > aReflowState.mComputedMaxWidth) {
      visibleWidth = aReflowState.mComputedMaxWidth - aReflowState.mComputedBorderPadding.left - aReflowState.mComputedBorderPadding.right;
    }
    if (fullWidth < aReflowState.mComputedMinWidth) {
      visibleWidth = aReflowState.mComputedMinWidth - aReflowState.mComputedBorderPadding.left - aReflowState.mComputedBorderPadding.right;
    }

    // calculate full height for comparison
    // must add in Border & Padding twice because the scrolled area also inherits Border & Padding
    nscoord fullHeight = 0;
    if (aReflowState.mComputedHeight != 0) {
      fullHeight = visibleHeight + aReflowState.mComputedBorderPadding.top + aReflowState.mComputedBorderPadding.bottom;
                                         // + aReflowState.mComputedBorderPadding.top + aReflowState.mComputedBorderPadding.bottom;
    }
    if (fullHeight > aReflowState.mComputedMaxHeight) {
      visibleHeight = aReflowState.mComputedMaxHeight - aReflowState.mComputedBorderPadding.top - aReflowState.mComputedBorderPadding.bottom;
    }
    if (fullHeight < aReflowState.mComputedMinHeight) {
      visibleHeight = aReflowState.mComputedMinHeight - aReflowState.mComputedBorderPadding.top - aReflowState.mComputedBorderPadding.bottom;
    }
  }

   // Do a second reflow with the adjusted width and height settings
   // This sets up all of the frames with the correct width and height.
  secondPassState.mComputedWidth  = visibleWidth;
  secondPassState.mComputedHeight = visibleHeight;
  secondPassState.reason = eReflowReason_Resize;

  if (mPassId == 0 || mPassId == 2 || visibleHeight != scrolledAreaHeight ||
      visibleWidth != scrolledAreaWidth) {
    nsHTMLScrollFrame::Reflow(aPresContext, aDesiredSize, secondPassState, aStatus);
    if (aReflowState.mComputedHeight == 0) {
      aDesiredSize.ascent  = 0;
      aDesiredSize.descent = 0;
      aDesiredSize.height  = 0;
    }

    // Set the max element size to be the same as the desired element size.
  } else {
    // aDesiredSize is the desired frame size, so includes border and padding
    aDesiredSize.width  = visibleWidth +
      (aReflowState.mComputedBorderPadding.left + aReflowState.mComputedBorderPadding.right);
    aDesiredSize.height = visibleHeight +
      (aReflowState.mComputedBorderPadding.top + aReflowState.mComputedBorderPadding.bottom);
    aDesiredSize.ascent =
      scrolledAreaDesiredSize.ascent + aReflowState.mComputedBorderPadding.top;
    aDesiredSize.descent = aDesiredSize.height - aDesiredSize.ascent;
  }

  if (aDesiredSize.mComputeMEW) {
    aDesiredSize.mMaxElementWidth = aDesiredSize.width;
  }

  aStatus = NS_FRAME_COMPLETE;

#ifdef DEBUG_rodsX
  if (!isInDropDownMode) {
    PRInt32 numRows = 1;
    GetSizeAttribute(&numRows);
    printf("%d ", numRows);
    if (numRows == 2) {
      COMPARE_QUIRK_SIZE("nsListControlFrame", 56, 38) 
    } else if (numRows == 3) {
      COMPARE_QUIRK_SIZE("nsListControlFrame", 56, 54) 
    } else if (numRows == 4) {
      COMPARE_QUIRK_SIZE("nsListControlFrame", 56, 70) 
    } else {
      COMPARE_QUIRK_SIZE("nsListControlFrame", 127, 118) 
    }
  }
#endif

  if (aReflowState.availableWidth != NS_UNCONSTRAINEDSIZE) {
    mCachedAvailableSize.width  = aDesiredSize.width - (aReflowState.mComputedBorderPadding.left + aReflowState.mComputedBorderPadding.right);
    REFLOW_DEBUG_MSG2("** nsLCF Caching AW: %d\n", PX(mCachedAvailableSize.width));
  }
  if (aReflowState.availableHeight != NS_UNCONSTRAINEDSIZE) {
    mCachedAvailableSize.height = aDesiredSize.height - (aReflowState.mComputedBorderPadding.top + aReflowState.mComputedBorderPadding.bottom);
    REFLOW_DEBUG_MSG2("** nsLCF Caching AH: %d\n", PX(mCachedAvailableSize.height));
  }

  //REFLOW_DEBUG_MSG3("** nsLCF Caching AW: %d  AH: %d\n", PX(mCachedAvailableSize.width), PX(mCachedAvailableSize.height));

  nsFormControlFrame::SetupCachedSizes(mCacheSize, mCachedAscent,
                                       mCachedMaxElementWidth, aDesiredSize);

  REFLOW_DEBUG_MSG3("** Done nsLCF DW: %d  DH: %d\n\n", PX(aDesiredSize.width), PX(aDesiredSize.height));

  REFLOW_COUNTER();

#ifdef DO_REFLOW_COUNTER
  if (gReflowControlCnt[mReflowId] > 50) {
    REFLOW_DEBUG_MSG3("** Id: %d Cnt: %d ", mReflowId, gReflowControlCnt[mReflowId]);
    REFLOW_DEBUG_MSG3("Done nsLCF DW: %d  DH: %d\n", PX(aDesiredSize.width), PX(aDesiredSize.height));
  }
#endif

  NS_ASSERTION(aDesiredSize.width < 100000, "Width is still NS_UNCONSTRAINEDSIZE");
  NS_ASSERTION(aDesiredSize.height < 100000, "Height is still NS_UNCONSTRAINEDSIZE");
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}

//---------------------------------------------------------
NS_IMETHODIMP
nsListControlFrame::GetFormContent(nsIContent*& aContent) const
{
  aContent = GetContent();
  NS_IF_ADDREF(aContent);
  return NS_OK;
}

nsGfxScrollFrameInner::ScrollbarStyles
nsListControlFrame::GetScrollbarStyles() const
{
  // We can't express this in the style system yet; when we can, this can go away
  // and GetScrollbarStyles can be devirtualized
  PRInt32 verticalStyle = IsInDropDownMode() ? NS_STYLE_OVERFLOW_AUTO
    : NS_STYLE_OVERFLOW_SCROLL;
  return nsGfxScrollFrameInner::ScrollbarStyles(NS_STYLE_OVERFLOW_HIDDEN,
                                                verticalStyle);
}

//---------------------------------------------------------
PRBool 
nsListControlFrame::IsOptionElement(nsIContent* aContent)
{
  PRBool result = PR_FALSE;
 
  nsCOMPtr<nsIDOMHTMLOptionElement> optElem;
  if (NS_SUCCEEDED(aContent->QueryInterface(NS_GET_IID(nsIDOMHTMLOptionElement),(void**) getter_AddRefs(optElem)))) {      
    if (optElem != nsnull) {
      result = PR_TRUE;
    }
  }
 
  return result;
}

nsIFrame*
nsListControlFrame::GetContentInsertionFrame() {
  nsIFrame* frame;
  GetOptionsContainer(GetPresContext(), &frame);
  return frame->GetContentInsertionFrame();
}

//---------------------------------------------------------
// Starts at the passed in content object and walks up the 
// parent heierarchy looking for the nsIDOMHTMLOptionElement
//---------------------------------------------------------
nsIContent *
nsListControlFrame::GetOptionFromContent(nsIContent *aContent) 
{
  for (nsIContent* content = aContent; content; content = content->GetParent()) {
    if (IsOptionElement(content)) {
      return content;
    }
  }

  return nsnull;
}

//---------------------------------------------------------
// Finds the index of the hit frame's content in the list
// of option elements
//---------------------------------------------------------
PRInt32 
nsListControlFrame::GetIndexFromContent(nsIContent *aContent)
{
  nsCOMPtr<nsIDOMHTMLOptionElement> option;
  option = do_QueryInterface(aContent);
  if (option) {
    PRInt32 retval;
    option->GetIndex(&retval);
    if (retval >= 0) {
      return retval;
    }
  }
  return kNothingSelected;
}

//---------------------------------------------------------
PRBool
nsListControlFrame::ExtendedSelection(PRInt32 aStartIndex,
                                      PRInt32 aEndIndex,
                                      PRBool aClearAll)
{
  return SetOptionsSelectedFromFrame(aStartIndex, aEndIndex,
                                     PR_TRUE, aClearAll);
}

//---------------------------------------------------------
PRBool
nsListControlFrame::SingleSelection(PRInt32 aClickedIndex, PRBool aDoToggle)
{
  if (mComboboxFrame) {
    PRInt32 selectedIndex;
    GetSelectedIndex(&selectedIndex);
    mComboboxFrame->UpdateRecentIndex(selectedIndex);
  }

  PRBool wasChanged = PR_FALSE;
  // Get Current selection
  if (aDoToggle) {
    wasChanged = ToggleOptionSelectedFromFrame(aClickedIndex);
  } else {
    wasChanged = SetOptionsSelectedFromFrame(aClickedIndex, aClickedIndex,
                                PR_TRUE, PR_TRUE);
  }
  ScrollToIndex(aClickedIndex);
  mStartSelectionIndex = aClickedIndex;
  mEndSelectionIndex = aClickedIndex;
  return wasChanged;
}

void
nsListControlFrame::InitSelectionRange(PRInt32 aClickedIndex)
{
  //
  // If nothing is selected, set the start selection depending on where
  // the user clicked and what the initial selection is:
  // - if the user clicked *before* selectedIndex, set the start index to
  //   the end of the first contiguous selection.
  // - if the user clicked *after* the end of the first contiguous
  //   selection, set the start index to selectedIndex.
  // - if the user clicked *within* the first contiguous selection, set the
  //   start index to selectedIndex.
  // The last two rules, of course, boil down to the same thing: if the user
  // clicked >= selectedIndex, return selectedIndex.
  //
  // This makes it so that shift click works properly when you first click
  // in a multiple select.
  //
  PRInt32 selectedIndex;
  GetSelectedIndex(&selectedIndex);
  if (selectedIndex >= 0) {
    // Get the end of the contiguous selection
    nsCOMPtr<nsIDOMHTMLOptionsCollection> options =
      getter_AddRefs(GetOptions(mContent));
    NS_ASSERTION(options, "Collection of options is null!");
    PRUint32 numOptions;
    options->GetLength(&numOptions);
    PRUint32 i;
    // Push i to one past the last selected index in the group
    for (i=selectedIndex+1; i < numOptions; i++) {
      PRBool selected;
      GetOption(options, i)->GetSelected(&selected);
      if (!selected) {
        break;
      }
    }

    if (aClickedIndex < selectedIndex) {
      // User clicked before selection, so start selection at end of
      // contiguous selection
      mStartSelectionIndex = i-1;
      mEndSelectionIndex = selectedIndex;
    } else {
      // User clicked after selection, so start selection at start of
      // contiguous selection
      mStartSelectionIndex = selectedIndex;
      mEndSelectionIndex = i-1;
    }
  }
}

//---------------------------------------------------------
PRBool
nsListControlFrame::PerformSelection(PRInt32 aClickedIndex,
                                     PRBool aIsShift,
                                     PRBool aIsControl)
{
  PRBool wasChanged = PR_FALSE;

  PRBool isMultiple;
  GetMultiple(&isMultiple);

  if (aClickedIndex == kNothingSelected) {
  } else if (isMultiple) {
    if (aIsShift) {
      // Make sure shift+click actually does something expected when
      // the user has never clicked on the select
      if (mStartSelectionIndex == kNothingSelected) {
        InitSelectionRange(aClickedIndex);
      }

      // Get the range from beginning (low) to end (high)
      // Shift *always* works, even if the current option is disabled
      PRInt32 startIndex;
      PRInt32 endIndex;
      if (mStartSelectionIndex == kNothingSelected) {
        startIndex = aClickedIndex;
        endIndex   = aClickedIndex;
      } else if (mStartSelectionIndex <= aClickedIndex) {
        startIndex = mStartSelectionIndex;
        endIndex   = aClickedIndex;
      } else {
        startIndex = aClickedIndex;
        endIndex   = mStartSelectionIndex;
      }

      // Clear only if control was not pressed
      wasChanged = ExtendedSelection(startIndex, endIndex, !aIsControl);
      ScrollToIndex(aClickedIndex);

      if (mStartSelectionIndex == kNothingSelected) {
        mStartSelectionIndex = aClickedIndex;
        mEndSelectionIndex = aClickedIndex;
      } else {
        mEndSelectionIndex = aClickedIndex;
      }
    } else if (aIsControl) {
      wasChanged = SingleSelection(aClickedIndex, PR_TRUE);
    } else {
      wasChanged = SingleSelection(aClickedIndex, PR_FALSE);
    }
  } else {
    wasChanged = SingleSelection(aClickedIndex, PR_FALSE);
  }

  return wasChanged;
}

//---------------------------------------------------------
PRBool
nsListControlFrame::HandleListSelection(nsIDOMEvent* aEvent,
                                        PRInt32 aClickedIndex)
{
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aEvent);
  PRBool isShift;
  PRBool isControl;
#if defined(XP_MAC) || defined(XP_MACOSX)
  mouseEvent->GetMetaKey(&isControl);
#else
  mouseEvent->GetCtrlKey(&isControl);
#endif
  mouseEvent->GetShiftKey(&isShift);
  return PerformSelection(aClickedIndex, isShift, isControl);
}

//---------------------------------------------------------
NS_IMETHODIMP
nsListControlFrame::CaptureMouseEvents(nsPresContext* aPresContext, PRBool aGrabMouseEvents)
{
  // Currently cocoa widgets use a native popup widget which tracks clicks synchronously,
  // so we never want to do mouse capturing. Note that we only bail if the list
  // is in drop-down mode, and the caller is requesting capture (we let release capture
  // requests go through to ensure that we can release capture requested via other
  // code paths, if any exist).
  if (aGrabMouseEvents && IsInDropDownMode() && nsComboboxControlFrame::ToolkitHasNativePopup())
    return NS_OK;

  nsIView* view = GetScrolledFrame()->GetView();

  NS_ASSERTION(view, "no view???");
  NS_ENSURE_TRUE(view, NS_ERROR_FAILURE);

  nsIViewManager* viewMan = view->GetViewManager();
  if (viewMan) {
    PRBool result;
    // It's not clear why we don't have the widget capture mouse events here.
    if (aGrabMouseEvents) {
      viewMan->GrabMouseEvents(view, result);
    } else {
      nsIView* curGrabber;
      viewMan->GetMouseEventGrabber(curGrabber);
      PRBool dropDownIsHidden = PR_FALSE;
      if (IsInDropDownMode()) {
        PRBool isDroppedDown;
        mComboboxFrame->IsDroppedDown(&isDroppedDown);
        dropDownIsHidden = !isDroppedDown;
      }
      if (curGrabber == view || dropDownIsHidden) {
        // only unset the grabber if *we* are the ones doing the grabbing
        // (or if the dropdown is hidden, in which case NO-ONE should be
        // grabbing anything
        // it could be a scrollbar inside this listbox which is actually grabbing
        // This shouldn't be necessary. We should simply ensure that events targeting
        // scrollbars are never visible to DOM consumers.
        viewMan->GrabMouseEvents(nsnull, result);
      }
    }
  }

  return NS_OK;
}

//---------------------------------------------------------
NS_IMETHODIMP 
nsListControlFrame::HandleEvent(nsPresContext* aPresContext, 
                                       nsGUIEvent*     aEvent,
                                       nsEventStatus*  aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  // temp fix until Bug 124990 gets fixed
  if (aPresContext->IsPaginated() && NS_IS_MOUSE_EVENT(aEvent)) {
    return NS_OK;
  }

  /*const char * desc[] = {"NS_MOUSE_MOVE", 
                          "NS_MOUSE_LEFT_BUTTON_UP",
                          "NS_MOUSE_LEFT_BUTTON_DOWN",
                          "<NA>","<NA>","<NA>","<NA>","<NA>","<NA>","<NA>",
                          "NS_MOUSE_MIDDLE_BUTTON_UP",
                          "NS_MOUSE_MIDDLE_BUTTON_DOWN",
                          "<NA>","<NA>","<NA>","<NA>","<NA>","<NA>","<NA>","<NA>",
                          "NS_MOUSE_RIGHT_BUTTON_UP",
                          "NS_MOUSE_RIGHT_BUTTON_DOWN",
                          "NS_MOUSE_ENTER_SYNTH",
                          "NS_MOUSE_EXIT_SYNTH",
                          "NS_MOUSE_LEFT_DOUBLECLICK",
                          "NS_MOUSE_MIDDLE_DOUBLECLICK",
                          "NS_MOUSE_RIGHT_DOUBLECLICK",
                          "NS_MOUSE_LEFT_CLICK",
                          "NS_MOUSE_MIDDLE_CLICK",
                          "NS_MOUSE_RIGHT_CLICK"};
  int inx = aEvent->message-NS_MOUSE_MESSAGE_START;
  if (inx >= 0 && inx <= (NS_MOUSE_RIGHT_CLICK-NS_MOUSE_MESSAGE_START)) {
    printf("Mouse in ListFrame %s [%d]\n", desc[inx], aEvent->message);
  } else {
    printf("Mouse in ListFrame <UNKNOWN> [%d]\n", aEvent->message);
  }*/

  if (nsEventStatus_eConsumeNoDefault == *aEventStatus)
    return NS_OK;

  // do we have style that affects how we are selected?
  // do we have user-input style?
  const nsStyleUserInterface* uiStyle = GetStyleUserInterface();
  if (uiStyle->mUserInput == NS_STYLE_USER_INPUT_NONE || uiStyle->mUserInput == NS_STYLE_USER_INPUT_DISABLED)
    return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);

  if (nsFormControlHelper::GetDisabled(mContent))
    return NS_OK;

  return nsHTMLScrollFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}


//---------------------------------------------------------
NS_IMETHODIMP
nsListControlFrame::SetInitialChildList(nsPresContext* aPresContext,
                                        nsIAtom*        aListName,
                                        nsIFrame*       aChildList)
{
  // First check to see if all the content has been added
  mIsAllContentHere = mContent->IsDoneAddingChildren();
  if (!mIsAllContentHere) {
    mIsAllFramesHere    = PR_FALSE;
    mHasBeenInitialized = PR_FALSE;
  }
  nsresult rv = nsHTMLScrollFrame::SetInitialChildList(aPresContext, aListName, aChildList);

  // If all the content is here now check
  // to see if all the frames have been created
  /*if (mIsAllContentHere) {
    // If all content and frames are here
    // the reset/initialize
    if (CheckIfAllFramesHere()) {
      ResetList(aPresContext);
      mHasBeenInitialized = PR_TRUE;
    }
  }*/

  return rv;
}

//---------------------------------------------------------
nsresult
nsListControlFrame::GetSizeAttribute(PRInt32 *aSize) {
  nsresult rv = NS_OK;
  nsIDOMHTMLSelectElement* selectElement;
  rv = mContent->QueryInterface(NS_GET_IID(nsIDOMHTMLSelectElement),(void**) &selectElement);
  if (mContent && NS_SUCCEEDED(rv)) {
    rv = selectElement->GetSize(aSize);
    NS_RELEASE(selectElement);
  }
  return rv;
}


//---------------------------------------------------------
NS_IMETHODIMP  
nsListControlFrame::Init(nsPresContext*  aPresContext,
                         nsIContent*      aContent,
                         nsIFrame*        aParent,
                         nsStyleContext*  aContext,
                         nsIFrame*        aPrevInFlow)
{
  nsresult result = nsHTMLScrollFrame::Init(aPresContext, aContent, aParent, aContext,
                                            aPrevInFlow);

  // get the receiver interface from the browser button's content node
  nsCOMPtr<nsIDOMEventReceiver> receiver(do_QueryInterface(mContent));

  // we shouldn't have to unregister this listener because when
  // our frame goes away all these content node go away as well
  // because our frame is the only one who references them.
  // we need to hook up our listeners before the editor is initialized
  mEventListener = new nsListEventListener(this);
  if (!mEventListener) 
    return NS_ERROR_OUT_OF_MEMORY;

  receiver->AddEventListenerByIID(NS_STATIC_CAST(nsIDOMMouseListener*,
                                                 mEventListener),
                                  NS_GET_IID(nsIDOMMouseListener));

  receiver->AddEventListenerByIID(NS_STATIC_CAST(nsIDOMMouseMotionListener*,
                                                 mEventListener),
                                  NS_GET_IID(nsIDOMMouseMotionListener));

  receiver->AddEventListenerByIID(NS_STATIC_CAST(nsIDOMKeyListener*,
                                                 mEventListener),
                                  NS_GET_IID(nsIDOMKeyListener));

  mStartSelectionIndex = kNothingSelected;
  mEndSelectionIndex = kNothingSelected;

  return result;
}


//---------------------------------------------------------
nscoord 
nsListControlFrame::GetVerticalInsidePadding(nsPresContext* aPresContext,
                                             float aPixToTwip, 
                                             nscoord aInnerHeight) const
{
   return NSIntPixelsToTwips(0, aPixToTwip); 
}


//---------------------------------------------------------
nscoord 
nsListControlFrame::GetHorizontalInsidePadding(nsPresContext* aPresContext,
                                               float aPixToTwip, 
                                               nscoord aInnerWidth,
                                               nscoord aCharWidth) const
{
  return GetVerticalInsidePadding(aPresContext, aPixToTwip, aInnerWidth);
}


NS_IMETHODIMP 
nsListControlFrame::GetMultiple(PRBool* aMultiple, nsIDOMHTMLSelectElement* aSelect)
{
  if (!aSelect) {
    nsIDOMHTMLSelectElement* selectElement = nsnull;
    nsresult result = mContent->QueryInterface(NS_GET_IID(nsIDOMHTMLSelectElement),
                                               (void**)&selectElement);
    if (NS_SUCCEEDED(result) && selectElement) {
      result = selectElement->GetMultiple(aMultiple);
      NS_RELEASE(selectElement);
    } 
    return result;
  } else {
    return aSelect->GetMultiple(aMultiple);
  }
}


//---------------------------------------------------------
// for a given piece of content it returns nsIDOMHTMLSelectElement object
// or null 
//---------------------------------------------------------
nsIDOMHTMLSelectElement* 
nsListControlFrame::GetSelect(nsIContent * aContent)
{
  nsIDOMHTMLSelectElement* selectElement = nsnull;
  nsresult result = aContent->QueryInterface(NS_GET_IID(nsIDOMHTMLSelectElement),
                                             (void**)&selectElement);
  if (NS_SUCCEEDED(result) && selectElement) {
    return selectElement;
  } else {
    return nsnull;
  }
}

already_AddRefed<nsIContent> 
nsListControlFrame::GetOptionAsContent(nsIDOMHTMLOptionsCollection* aCollection, PRInt32 aIndex) 
{
  nsIContent * content = nsnull;
  nsCOMPtr<nsIDOMHTMLOptionElement> optionElement = getter_AddRefs(GetOption(aCollection, aIndex));

  NS_ASSERTION(optionElement != nsnull, "could not get option element by index!");

  if (optionElement) {
    CallQueryInterface(optionElement, &content);
  }
 
  return content;
}

already_AddRefed<nsIContent> 
nsListControlFrame::GetOptionContent(PRInt32 aIndex)
  
{
  nsCOMPtr<nsIDOMHTMLOptionsCollection> options =
    getter_AddRefs(GetOptions(mContent));
  NS_ASSERTION(options.get() != nsnull, "Collection of options is null!");

  if (options) {
    return GetOptionAsContent(options, aIndex);
  } 
  return nsnull;
}

nsIDOMHTMLOptionsCollection* 
nsListControlFrame::GetOptions(nsIContent * aContent, nsIDOMHTMLSelectElement* aSelect)
{
  nsIDOMHTMLOptionsCollection* options = nsnull;
  if (!aSelect) {
    nsCOMPtr<nsIDOMHTMLSelectElement> selectElement = getter_AddRefs(GetSelect(aContent));
    if (selectElement) {
      selectElement->GetOptions(&options);  // AddRefs (1)
    }
  } else {
    aSelect->GetOptions(&options); // AddRefs (1)
  }

  return options;
}

nsIDOMHTMLOptionElement*
nsListControlFrame::GetOption(nsIDOMHTMLOptionsCollection* aCollection,
                              PRInt32 aIndex)
{
  nsCOMPtr<nsIDOMNode> node;
  if (NS_SUCCEEDED(aCollection->Item(aIndex, getter_AddRefs(node)))) {
    NS_ASSERTION(node,
                 "Item was successful, but node from collection was null!");
    if (node) {
      nsIDOMHTMLOptionElement* option = nsnull;
      CallQueryInterface(node, &option);

      return option;
    }
  } else {
    NS_ERROR("Couldn't get option by index from collection!");
  }
  return nsnull;
}

PRBool 
nsListControlFrame::IsContentSelected(nsIContent* aContent)
{
  PRBool isSelected = PR_FALSE;

  nsCOMPtr<nsIDOMHTMLOptionElement> optEl = do_QueryInterface(aContent);
  if (optEl)
    optEl->GetSelected(&isSelected);

  return isSelected;
}

PRBool 
nsListControlFrame::IsContentSelectedByIndex(PRInt32 aIndex) 
{
  nsCOMPtr<nsIContent> content = GetOptionContent(aIndex);
  NS_ASSERTION(content, "Failed to retrieve option content");

  return IsContentSelected(content);
}

NS_IMETHODIMP
nsListControlFrame::OnOptionSelected(nsPresContext* aPresContext,
                                     PRInt32 aIndex,
                                     PRBool aSelected)
{
  if (aSelected) {
    ScrollToIndex(aIndex);
  }
  return NS_OK;
}

PRIntn
nsListControlFrame::GetSkipSides() const
{    
    // Don't skip any sides during border rendering
  return 0;
}

NS_IMETHODIMP_(PRInt32)
nsListControlFrame::GetFormControlType() const
{
  return NS_FORM_SELECT;
}

NS_IMETHODIMP
nsListControlFrame::OnContentReset()
{
  ResetList(PR_TRUE);
  return NS_OK;
}

void 
nsListControlFrame::ResetList(PRBool aAllowScrolling)
{
  REFLOW_DEBUG_MSG("LBX::ResetList\n");

  // if all the frames aren't here 
  // don't bother reseting
  if (!mIsAllFramesHere) {
    return;
  }

  if (aAllowScrolling) {
    mPostChildrenLoadedReset = PR_TRUE;

    // Scroll to the selected index
    PRInt32 indexToSelect = kNothingSelected;

    nsCOMPtr<nsIDOMHTMLSelectElement> selectElement(do_QueryInterface(mContent));
    NS_ASSERTION(selectElement, "No select element!");
    if (selectElement) {
      selectElement->GetSelectedIndex(&indexToSelect);
      ScrollToIndex(indexToSelect);
    }
  }

  mStartSelectionIndex = kNothingSelected;
  mEndSelectionIndex = kNothingSelected;

  // Combobox will redisplay itself with the OnOptionSelected event
} 

//---------------------------------------------------------
NS_IMETHODIMP
nsListControlFrame::GetName(nsAString* aResult)
{
  return nsFormControlHelper::GetName(mContent, aResult);
}
 

void 
nsListControlFrame::SetFocus(PRBool aOn, PRBool aRepaint)
{
  if (aOn) {
    ComboboxFocusSet();
    PRInt32 selectedIndex;
    GetSelectedIndex(&selectedIndex);
    mFocused = this;
  } else {
    mFocused = nsnull;
  }

  // Make sure the SelectArea frame gets painted
  Invalidate(nsRect(0,0,mRect.width,mRect.height), PR_TRUE);
}

void nsListControlFrame::ComboboxFocusSet()
{
  gLastKeyTime = 0;
}

void 
nsListControlFrame::ScrollIntoView(nsPresContext* aPresContext)
{
  if (aPresContext) {
    nsIPresShell *presShell = aPresContext->GetPresShell();
    if (presShell) {
      presShell->ScrollFrameIntoView(this,
                   NS_PRESSHELL_SCROLL_IF_NOT_VISIBLE,NS_PRESSHELL_SCROLL_IF_NOT_VISIBLE);
    }
  }
}


//---------------------------------------------------------
NS_IMETHODIMP 
nsListControlFrame::SetComboboxFrame(nsIFrame* aComboboxFrame)
{
  nsresult rv = NS_OK;
  if (nsnull != aComboboxFrame) {
    rv = aComboboxFrame->QueryInterface(NS_GET_IID(nsIComboboxControlFrame),(void**) &mComboboxFrame); 
  }
  return rv;
}

NS_IMETHODIMP 
nsListControlFrame::GetOptionText(PRInt32 aIndex, nsAString & aStr)
{
  aStr.SetLength(0);
  nsresult rv = NS_ERROR_FAILURE; 
  nsCOMPtr<nsIDOMHTMLOptionsCollection> options =
    getter_AddRefs(GetOptions(mContent));

  if (options) {
    PRUint32 numOptions;
    options->GetLength(&numOptions);

    if (numOptions == 0) {
      rv = NS_OK;
    } else {
      nsCOMPtr<nsIDOMHTMLOptionElement> optionElement(
          getter_AddRefs(GetOption(options, aIndex)));
      if (optionElement) {
#if 0 // This is for turning off labels Bug 4050
        nsAutoString text;
        rv = optionElement->GetLabel(text);
        // the return value is always NS_OK from DOMElements
        // it is meaningless to check for it
        if (!text.IsEmpty()) { 
          nsAutoString compressText = text;
          compressText.CompressWhitespace(PR_TRUE, PR_TRUE);
          if (!compressText.IsEmpty()) {
            text = compressText;
          }
        }

        if (text.IsEmpty()) {
          // the return value is always NS_OK from DOMElements
          // it is meaningless to check for it
          optionElement->GetText(text);
        }          
        aStr = text;
#else
        optionElement->GetText(aStr);
#endif
        rv = NS_OK;
      }
    }
  }

  return rv;
}

NS_IMETHODIMP 
nsListControlFrame::GetSelectedIndex(PRInt32 * aIndex)
{
  nsCOMPtr<nsIDOMHTMLSelectElement> selectElement(do_QueryInterface(mContent));
  return selectElement->GetSelectedIndex(aIndex);
}

PRBool 
nsListControlFrame::IsInDropDownMode() const
{
  return (mComboboxFrame != nsnull);
}

NS_IMETHODIMP 
nsListControlFrame::GetNumberOfOptions(PRInt32* aNumOptions) 
{
  if (mContent != nsnull) {
    nsCOMPtr<nsIDOMHTMLOptionsCollection> options =
      getter_AddRefs(GetOptions(mContent));

    if (nsnull == options) {
      *aNumOptions = 0;
    } else {
      PRUint32 length = 0;
      options->GetLength(&length);
      *aNumOptions = (PRInt32)length;
    }
    return NS_OK;
  }
  *aNumOptions = 0;
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// nsISelectControlFrame
//----------------------------------------------------------------------
PRBool nsListControlFrame::CheckIfAllFramesHere()
{
  // Get the number of optgroups and options
  //PRInt32 numContentItems = 0;
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mContent));
  if (node) {
    // XXX Need to find a fail proff way to determine that
    // all the frames are there
    mIsAllFramesHere = PR_TRUE;//NS_OK == CountAllChild(node, numContentItems);
  }
  // now make sure we have a frame each piece of content

  return mIsAllFramesHere;
}

NS_IMETHODIMP
nsListControlFrame::DoneAddingChildren(PRBool aIsDone)
{
  mIsAllContentHere = aIsDone;
  if (mIsAllContentHere) {
    // Here we check to see if all the frames have been created 
    // for all the content.
    // If so, then we can initialize;
    if (mIsAllFramesHere == PR_FALSE) {
      // if all the frames are now present we can initalize
      if (CheckIfAllFramesHere()) {
        mHasBeenInitialized = PR_TRUE;
        ResetList(PR_TRUE);
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsListControlFrame::AddOption(nsPresContext* aPresContext, PRInt32 aIndex)
{
#ifdef DO_REFLOW_DEBUG
  printf("---- Id: %d nsLCF %p Added Option %d\n", mReflowId, this, aIndex);
#endif

  PRInt32 numOptions;
  GetNumberOfOptions(&numOptions);

  if (!mIsAllContentHere) {
    mIsAllContentHere = mContent->IsDoneAddingChildren();
    if (!mIsAllContentHere) {
      mIsAllFramesHere    = PR_FALSE;
      mHasBeenInitialized = PR_FALSE;
    } else {
      mIsAllFramesHere = aIndex == numOptions-1;
    }
  }
  
  if (!mHasBeenInitialized) {
    return NS_OK;
  }

  // Make sure we scroll to the selected option as needed
  mNeedToReset = PR_TRUE;
  mPostChildrenLoadedReset = mIsAllContentHere;
  return NS_OK;
}

NS_IMETHODIMP
nsListControlFrame::RemoveOption(nsPresContext* aPresContext, PRInt32 aIndex)
{
  // Need to reset if we're a dropdown
  if (IsInDropDownMode()) {
    mNeedToReset = PR_TRUE;
    mPostChildrenLoadedReset = mIsAllContentHere;
  }

  return NS_OK;
}

//---------------------------------------------------------
// Set the option selected in the DOM.  This method is named
// as it is because it indicates that the frame is the source
// of this event rather than the receiver.
PRBool
nsListControlFrame::SetOptionsSelectedFromFrame(PRInt32 aStartIndex,
                                                PRInt32 aEndIndex,
                                                PRBool aValue,
                                                PRBool aClearAll)
{
  nsCOMPtr<nsISelectElement> selectElement(do_QueryInterface(mContent));
  PRBool wasChanged = PR_FALSE;
  nsresult rv = selectElement->SetOptionsSelectedByIndex(aStartIndex,
                                                         aEndIndex,
                                                         aValue,
                                                         aClearAll,
                                                         PR_FALSE,
                                                         PR_TRUE,
                                                         &wasChanged);
  NS_ASSERTION(NS_SUCCEEDED(rv), "SetSelected failed");
  return wasChanged;
}

PRBool
nsListControlFrame::ToggleOptionSelectedFromFrame(PRInt32 aIndex)
{
  nsCOMPtr<nsIDOMHTMLOptionsCollection> options =
    getter_AddRefs(GetOptions(mContent));
  NS_ASSERTION(options, "No options");
  if (!options) {
    return PR_FALSE;
  }
  nsCOMPtr<nsIDOMHTMLOptionElement> option(
      getter_AddRefs(GetOption(options, aIndex)));
  NS_ASSERTION(option, "No option");
  if (!option) {
    return PR_FALSE;
  }

  PRBool value = PR_FALSE;
  nsresult rv = option->GetSelected(&value);

  NS_ASSERTION(NS_SUCCEEDED(rv), "GetSelected failed");
  nsCOMPtr<nsISelectElement> selectElement(do_QueryInterface(mContent));
  PRBool wasChanged = PR_FALSE;
  rv = selectElement->SetOptionsSelectedByIndex(aIndex,
                                                aIndex,
                                                !value,
                                                PR_FALSE,
                                                PR_FALSE,
                                                PR_TRUE,
                                                &wasChanged);

  NS_ASSERTION(NS_SUCCEEDED(rv), "SetSelected failed");

  return wasChanged;
}


// Dispatch event and such
PRBool
nsListControlFrame::UpdateSelection()
{
  if (mIsAllFramesHere) {
    // if it's a combobox, display the new text
    if (mComboboxFrame) {
      mComboboxFrame->RedisplaySelectedText();
    }
    // if it's a listbox, fire on change
    else if (mIsAllContentHere) {
      nsWeakFrame weakFrame(this);
      FireOnChange();
      return weakFrame.IsAlive();
    }
  }
  return PR_TRUE;
}

NS_IMETHODIMP
nsListControlFrame::ComboboxFinish(PRInt32 aIndex)
{
  gLastKeyTime = 0;

  if (mComboboxFrame) {
    PerformSelection(aIndex, PR_FALSE, PR_FALSE);

    PRInt32 displayIndex;
    mComboboxFrame->GetIndexOfDisplayArea(&displayIndex);

    if (displayIndex != aIndex) {
      mComboboxFrame->RedisplaySelectedText();
    }

    mComboboxFrame->RollupFromList(GetPresContext()); // might destroy us
  }

  return NS_OK;
}

NS_IMETHODIMP
nsListControlFrame::GetOptionsContainer(nsPresContext* aPresContext,
                                        nsIFrame** aFrame)
{
  *aFrame = GetScrolledFrame();
  return NS_OK;
}

// Send out an onchange notification.
NS_IMETHODIMP
nsListControlFrame::FireOnChange()
{
  nsresult rv = NS_OK;
  
  if (mComboboxFrame) {
    // Return hit without changing anything
    PRInt32 index = mComboboxFrame->UpdateRecentIndex(-1);
    if (index == -1)
      return NS_OK;

    // See if the selection actually changed
    PRInt32 selectedIndex;
    GetSelectedIndex(&selectedIndex);
    if (index == selectedIndex)
      return NS_OK;
  }

  // Dispatch the NS_FORM_CHANGE event
  nsEventStatus status = nsEventStatus_eIgnore;
  nsEvent event(PR_TRUE, NS_FORM_CHANGE);

  nsCOMPtr<nsIPresShell> presShell = GetPresContext()->GetPresShell();
  if (presShell) {
    rv = presShell->HandleEventWithTarget(&event, this, nsnull,
                                           NS_EVENT_FLAG_INIT, &status);
  }

  return rv;
}

// Determine if the specified item in the listbox is selected.
NS_IMETHODIMP
nsListControlFrame::GetOptionSelected(PRInt32 aIndex, PRBool* aValue)
{
  *aValue = IsContentSelectedByIndex(aIndex);
  return NS_OK;
}

//---------------------------------------------------------
// Used by layout to determine if we have a fake option
NS_IMETHODIMP
nsListControlFrame::GetDummyFrame(nsIFrame** aFrame)
{
  (*aFrame) = mDummyFrame;
  return NS_OK;
}

NS_IMETHODIMP
nsListControlFrame::SetDummyFrame(nsIFrame* aFrame)
{
  mDummyFrame = aFrame;
  return NS_OK;
}

NS_IMETHODIMP
nsListControlFrame::OnSetSelectedIndex(PRInt32 aOldIndex, PRInt32 aNewIndex)
{
  if (mComboboxFrame) {
    // UpdateRecentIndex with -1, so that we won't fire an onchange
    // event for this setting of selectedIndex.
    mComboboxFrame->UpdateRecentIndex(-1);
  }

  ScrollToIndex(aNewIndex);
  mStartSelectionIndex = aNewIndex;
  mEndSelectionIndex = aNewIndex;

#ifdef ACCESSIBILITY
  FireMenuItemActiveEvent();
#endif

  return NS_OK;
}

//----------------------------------------------------------------------
// End nsISelectControlFrame
//----------------------------------------------------------------------

NS_IMETHODIMP 
nsListControlFrame::SetProperty(nsPresContext* aPresContext, nsIAtom* aName,
                                const nsAString& aValue)
{
  if (nsHTMLAtoms::selected == aName) {
    return NS_ERROR_INVALID_ARG; // Selected is readonly according to spec.
  } else if (nsHTMLAtoms::selectedindex == aName) {
    // You shouldn't be calling me for this!!!
    return NS_ERROR_INVALID_ARG;
  }

  // We should be told about selectedIndex by the DOM element through
  // OnOptionSelected

  return NS_OK;
}

NS_IMETHODIMP 
nsListControlFrame::GetProperty(nsIAtom* aName, nsAString& aValue)
{
  // Get the selected value of option from local cache (optimization vs. widget)
  if (nsHTMLAtoms::selected == aName) {
    nsAutoString val(aValue);
    PRInt32 error = 0;
    PRBool selected = PR_FALSE;
    PRInt32 indx = val.ToInteger(&error, 10); // Get index from aValue
    if (error == 0)
       selected = IsContentSelectedByIndex(indx); 
  
    nsFormControlHelper::GetBoolString(selected, aValue);
    
  // For selectedIndex, get the value from the widget
  } else if (nsHTMLAtoms::selectedindex == aName) {
    // You shouldn't be calling me for this!!!
    return NS_ERROR_INVALID_ARG;
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsListControlFrame::SyncViewWithFrame()
{
    // Resync the view's position with the frame.
    // The problem is the dropdown's view is attached directly under
    // the root view. This means it's view needs to have it's coordinates calculated
    // as if it were in it's normal position in the view hierarchy.
  mComboboxFrame->AbsolutelyPositionDropDown();

  nsContainerFrame::PositionFrameView(this);

  return NS_OK;
}

NS_IMETHODIMP 
nsListControlFrame::AboutToDropDown()
{
  if (mIsAllContentHere && mIsAllFramesHere && mHasBeenInitialized) {
    PRInt32 selectedIndex;
    GetSelectedIndex(&selectedIndex);
    ScrollToIndex(selectedIndex);
#ifdef ACCESSIBILITY
    FireMenuItemActiveEvent(); // Inform assistive tech what got focus
#endif
  }
  mItemSelectionStarted = PR_FALSE;

  return NS_OK;
}

// We are about to be rolledup from the outside (ComboboxFrame)
NS_IMETHODIMP 
nsListControlFrame::AboutToRollup()
{
  // We've been updating the combobox with the keyboard up until now, but not
  // with the mouse.  The problem is, even with mouse selection, we are
  // updating the <select>.  So if the mouse goes over an option just before
  // he leaves the box and clicks, that's what the <select> will show.
  //
  // To deal with this we say "whatever is in the combobox is canonical."
  // - IF the combobox is different from the current selected index, we
  //   reset the index.

  if (IsInDropDownMode()) {
    PRInt32 index;
    mComboboxFrame->GetIndexOfDisplayArea(&index);
    ComboboxFinish(index); // might destroy us
  }
  return NS_OK;
}

NS_IMETHODIMP
nsListControlFrame::DidReflow(nsPresContext*           aPresContext,
                              const nsHTMLReflowState*  aReflowState,
                              nsDidReflowStatus         aStatus)
{
  nsresult rv;
  
  if (IsInDropDownMode()) 
  {
    //SyncViewWithFrame();
    rv = nsHTMLScrollFrame::DidReflow(aPresContext, aReflowState, aStatus);
    SyncViewWithFrame();
  } else {
    rv = nsHTMLScrollFrame::DidReflow(aPresContext, aReflowState, aStatus);
  }

  if (mNeedToReset) {
    mNeedToReset = PR_FALSE;
    // Suppress scrolling to the selected element if we restored
    // scroll history state AND the list contents have not changed
    // since we loaded all the children AND nothing else forced us
    // to scroll by calling ResetList(PR_TRUE). The latter two conditions
    // are folded into mPostChildrenLoadedReset.
    //
    // The idea is that we want scroll history restoration to trump ResetList
    // scrolling to the selected element, when the ResetList was probably only
    // caused by content loading normally.
    ResetList(!DidHistoryRestore() || mPostChildrenLoadedReset);
  }

  return rv;
}

nsIAtom*
nsListControlFrame::GetType() const
{
  return nsLayoutAtoms::listControlFrame; 
}

#ifdef DEBUG
NS_IMETHODIMP
nsListControlFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("ListControl"), aResult);
}
#endif

//---------------------------------------------------------
NS_IMETHODIMP 
nsListControlFrame::GetMaximumSize(nsSize &aSize)
{
  aSize.width  = mMaxWidth;
  aSize.height = mMaxHeight;
  return NS_OK;
}


NS_IMETHODIMP 
nsListControlFrame::SetSuggestedSize(nscoord aWidth, nscoord aHeight)
{
  return NS_OK;
}

nsresult
nsListControlFrame::IsOptionDisabled(PRInt32 anIndex, PRBool &aIsDisabled)
{
  nsCOMPtr<nsISelectElement> sel(do_QueryInterface(mContent));
  if (sel) {
    sel->IsOptionDisabled(anIndex, &aIsDisabled);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// helper
//----------------------------------------------------------------------
PRBool
nsListControlFrame::IsLeftButton(nsIDOMEvent* aMouseEvent)
{
  // only allow selection with the left button
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aMouseEvent);
  if (mouseEvent) {
    PRUint16 whichButton;
    if (NS_SUCCEEDED(mouseEvent->GetButton(&whichButton))) {
      return whichButton != 0?PR_FALSE:PR_TRUE;
    }
  }
  return PR_FALSE;
}

//----------------------------------------------------------------------
// nsIDOMMouseListener
//----------------------------------------------------------------------
nsresult
nsListControlFrame::MouseUp(nsIDOMEvent* aMouseEvent)
{
  NS_ASSERTION(aMouseEvent != nsnull, "aMouseEvent is null.");

  UpdateInListState(aMouseEvent);

  REFLOW_DEBUG_MSG("--------------------------- MouseUp ----------------------------\n");

  mButtonDown = PR_FALSE;

  if (nsFormControlHelper::GetDisabled(mContent)) {
    return NS_OK;
  }

  // only allow selection with the left button
  // if a right button click is on the combobox itself
  // or on the select when in listbox mode, then let the click through
  if (!IsLeftButton(aMouseEvent)) {
    if (IsInDropDownMode()) {
      if (!IgnoreMouseEventForSelection(aMouseEvent)) {
        aMouseEvent->PreventDefault();

        nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aMouseEvent));

        if (nsevent) {
          nsevent->PreventCapture();
          nsevent->PreventBubble();
        }
      } else {
        CaptureMouseEvents(GetPresContext(), PR_FALSE);
        return NS_OK;
      }
      CaptureMouseEvents(GetPresContext(), PR_FALSE);
      return NS_ERROR_FAILURE; // means consume event
    } else {
      CaptureMouseEvents(GetPresContext(), PR_FALSE);
      return NS_OK;
    }
  }

  const nsStyleVisibility* vis = GetStyleVisibility();
      
  if (!vis->IsVisible()) {
    REFLOW_DEBUG_MSG(">>>>>> Select is NOT visible");
    return NS_OK;
  }

  if (IsInDropDownMode()) {
    // XXX This is a bit of a hack, but.....
    // But the idea here is to make sure you get an "onclick" event when you mouse
    // down on the select and the drag over an option and let go
    // And then NOT get an "onclick" event when when you click down on the select
    // and then up outside of the select
    // the EventStateManager tracks the content of the mouse down and the mouse up
    // to make sure they are the same, and the onclick is sent in the PostHandleEvent
    // depeneding on whether the clickCount is non-zero.
    // So we cheat here by either setting or unsetting the clcikCount in the native event
    // so the right thing happens for the onclick event
    nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aMouseEvent));
    nsMouseEvent * mouseEvent;
    privateEvent->GetInternalNSEvent((nsEvent**)&mouseEvent);

    PRInt32 selectedIndex;
    if (NS_SUCCEEDED(GetIndexFromDOMEvent(aMouseEvent, selectedIndex))) {
      REFLOW_DEBUG_MSG2(">>>>>> Found Index: %d", selectedIndex);

      // If it's disabled, disallow the click and leave.
      PRBool isDisabled = PR_FALSE;
      IsOptionDisabled(selectedIndex, isDisabled);
      if (isDisabled) {
        aMouseEvent->PreventDefault();

        nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aMouseEvent));

        if (nsevent) {
          nsevent->PreventCapture();
          nsevent->PreventBubble();
        }

        CaptureMouseEvents(GetPresContext(), PR_FALSE);
        return NS_ERROR_FAILURE;
      }

      if (kNothingSelected != selectedIndex) {
        nsWeakFrame weakFrame(this);
        ComboboxFinish(selectedIndex);
        if (!weakFrame.IsAlive())
          return NS_OK;
        FireOnChange();
      }

      mouseEvent->clickCount = 1;
    } else {
      // the click was out side of the select or its dropdown
      mouseEvent->clickCount = IgnoreMouseEventForSelection(aMouseEvent) ? 1 : 0;
    }
  } else {
    REFLOW_DEBUG_MSG(">>>>>> Didn't find");
    CaptureMouseEvents(GetPresContext(), PR_FALSE);
    // Notify
    if (mChangesSinceDragStart) {
      // reset this so that future MouseUps without a prior MouseDown
      // won't fire onchange
      mChangesSinceDragStart = PR_FALSE;
      FireOnChange();
    }
  }

  return NS_OK;
}

void
nsListControlFrame::UpdateInListState(nsIDOMEvent* aEvent)
{
  if (!mComboboxFrame)
    return;

  PRBool isDroppedDown;
  mComboboxFrame->IsDroppedDown(&isDroppedDown);
  if (!isDroppedDown)
    return;

  nsPoint pt = nsLayoutUtils::GetDOMEventCoordinatesRelativeTo(aEvent, this);
  nsRect borderInnerEdge = GetScrollableView()->View()->GetBounds();
  if (pt.y >= borderInnerEdge.y && pt.y < borderInnerEdge.YMost()) {
    mItemSelectionStarted = PR_TRUE;
  }
}

PRBool nsListControlFrame::IgnoreMouseEventForSelection(nsIDOMEvent* aEvent)
{
  if (!mComboboxFrame)
    return PR_FALSE;

  // Our DOM listener does get called when the dropdown is not
  // showing, because it listens to events on the SELECT element
  PRBool isDroppedDown;
  mComboboxFrame->IsDroppedDown(&isDroppedDown);
  if (!isDroppedDown)
    return PR_TRUE;

  return !mItemSelectionStarted;
}

#ifdef ACCESSIBILITY
void
nsListControlFrame::FireMenuItemActiveEvent()
{
  if (mFocused != this && !IsInDropDownMode()) {
    return;
  }

  // The mEndSelectionIndex is what is currently being selected
  // use the selected index if this is kNothingSelected
  PRInt32 focusedIndex;
  if (mEndSelectionIndex == kNothingSelected) {
    GetSelectedIndex(&focusedIndex);
  } else {
    focusedIndex = mEndSelectionIndex;
  }
  if (focusedIndex == kNothingSelected) {
    return;
  }

  nsCOMPtr<nsIContent> optionContent = GetOptionContent(focusedIndex);
  if (!optionContent) {
    return;
  }

  FireDOMEvent(NS_LITERAL_STRING("DOMMenuItemActive"), optionContent);
}
#endif

nsresult
nsListControlFrame::GetIndexFromDOMEvent(nsIDOMEvent* aMouseEvent, 
                                         PRInt32&     aCurIndex)
{
  if (IgnoreMouseEventForSelection(aMouseEvent)) {
    return NS_ERROR_FAILURE;
  }

  nsIView* view = GetScrolledFrame()->GetView();
  nsIViewManager* viewMan = view->GetViewManager();
  nsIView* curGrabber;
  viewMan->GetMouseEventGrabber(curGrabber);
  if (curGrabber != view) {
    // If we're not capturing, then ignore movement in the border
    nsPoint pt = nsLayoutUtils::GetDOMEventCoordinatesRelativeTo(aMouseEvent, this);
    nsRect borderInnerEdge = GetScrollableView()->View()->GetBounds();
    if (!borderInnerEdge.Contains(pt)) {
      return NS_ERROR_FAILURE;
    }
  }

  nsCOMPtr<nsIContent> content;
  GetPresContext()->EventStateManager()->
    GetEventTargetContent(nsnull, getter_AddRefs(content));

  nsCOMPtr<nsIContent> optionContent = GetOptionFromContent(content);
  if (optionContent) {
    aCurIndex = GetIndexFromContent(optionContent);
    return NS_OK;
  }

  nsIPresShell *presShell = GetPresContext()->PresShell();
  PRInt32 numOptions;
  GetNumberOfOptions(&numOptions);
  if (numOptions < 1)
    return NS_ERROR_FAILURE;

  nsPoint pt = nsLayoutUtils::GetDOMEventCoordinatesRelativeTo(aMouseEvent, this);

  // If the event coordinate is above the first option frame, then target the
  // first option frame
  nsCOMPtr<nsIContent> firstOption = GetOptionContent(0);
  NS_ASSERTION(firstOption, "Can't find first option that's supposed to be there");
  nsIFrame* optionFrame;
  nsresult rv = presShell->GetPrimaryFrameFor(firstOption, &optionFrame);
  if (NS_SUCCEEDED(rv) && optionFrame) {
    nsPoint ptInOptionFrame = pt - optionFrame->GetOffsetTo(this);
    if (ptInOptionFrame.y < 0 && ptInOptionFrame.x >= 0 &&
        ptInOptionFrame.x < optionFrame->GetSize().width) {
      aCurIndex = 0;
      return NS_OK;
    }
  }

  nsCOMPtr<nsIContent> lastOption = GetOptionContent(numOptions - 1);
  // If the event coordinate is below the last option frame, then target the
  // last option frame
  NS_ASSERTION(lastOption, "Can't find last option that's supposed to be there");
  rv = presShell->GetPrimaryFrameFor(lastOption, &optionFrame);
  if (NS_SUCCEEDED(rv) && optionFrame) {
    nsPoint ptInOptionFrame = pt - optionFrame->GetOffsetTo(this);
    if (ptInOptionFrame.y >= optionFrame->GetSize().height && ptInOptionFrame.x >= 0 &&
        ptInOptionFrame.x < optionFrame->GetSize().width) {
      aCurIndex = numOptions - 1;
      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}

nsresult
nsListControlFrame::MouseDown(nsIDOMEvent* aMouseEvent)
{
  NS_ASSERTION(aMouseEvent != nsnull, "aMouseEvent is null.");

  UpdateInListState(aMouseEvent);

  REFLOW_DEBUG_MSG("--------------------------- MouseDown ----------------------------\n");

  mButtonDown = PR_TRUE;

  if (nsFormControlHelper::GetDisabled(mContent)) {
    return NS_OK;
  }

  // only allow selection with the left button
  // if a right button click is on the combobox itself
  // or on the select when in listbox mode, then let the click through
  if (!IsLeftButton(aMouseEvent)) {
    if (IsInDropDownMode()) {
      if (!IgnoreMouseEventForSelection(aMouseEvent)) {
        aMouseEvent->PreventDefault();

        nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aMouseEvent));

        if (nsevent) {
          nsevent->PreventCapture();
          nsevent->PreventBubble();
        }
      } else {
        return NS_OK;
      }
      return NS_ERROR_FAILURE; // means consume event
    } else {
      return NS_OK;
    }
  }

  PRInt32 selectedIndex;
  if (NS_SUCCEEDED(GetIndexFromDOMEvent(aMouseEvent, selectedIndex))) {
    // Handle Like List
    CaptureMouseEvents(GetPresContext(), PR_TRUE);
    mChangesSinceDragStart = HandleListSelection(aMouseEvent, selectedIndex);
#ifdef ACCESSIBILITY
    if (mChangesSinceDragStart) {
      FireMenuItemActiveEvent();
    }
#endif
  } else {
    // NOTE: the combo box is responsible for dropping it down
    if (mComboboxFrame) {
      if (!IgnoreMouseEventForSelection(aMouseEvent)) {
        return NS_OK;
      }

      if (!nsComboboxControlFrame::ToolkitHasNativePopup())
      {
        PRBool isDroppedDown;
        mComboboxFrame->IsDroppedDown(&isDroppedDown);
        nsIFrame* comboFrame;
        CallQueryInterface(mComboboxFrame, &comboFrame);
        nsWeakFrame weakFrame(comboFrame);
        mComboboxFrame->ShowDropDown(!isDroppedDown);
        if (!weakFrame.IsAlive())
          return NS_OK;
        if (isDroppedDown) {
          CaptureMouseEvents(GetPresContext(), PR_FALSE);
        }
      }
    }
  }

  return NS_OK;
}

//----------------------------------------------------------------------
// nsIDOMMouseMotionListener
//----------------------------------------------------------------------
nsresult
nsListControlFrame::MouseMove(nsIDOMEvent* aMouseEvent)
{
  NS_ASSERTION(aMouseEvent, "aMouseEvent is null.");
  //REFLOW_DEBUG_MSG("MouseMove\n");

  UpdateInListState(aMouseEvent);

  if (IsInDropDownMode()) { 
    PRBool isDroppedDown = PR_FALSE;
    mComboboxFrame->IsDroppedDown(&isDroppedDown);
    if (isDroppedDown) {
      PRInt32 selectedIndex;
      if (NS_SUCCEEDED(GetIndexFromDOMEvent(aMouseEvent, selectedIndex))) {
        PerformSelection(selectedIndex, PR_FALSE, PR_FALSE);
      }

      // Make sure the SelectArea frame gets painted
      // XXX this shouldn't be needed, but other places in this code do it
      // and if we don't do this, invalidation doesn't happen when we move out
      // of the top-level window. We should track this down and fix it --- roc
      Invalidate(nsRect(0,0,mRect.width,mRect.height), PR_TRUE);
    }
  } else {// XXX - temporary until we get drag events
    if (mButtonDown) {
      return DragMove(aMouseEvent);
    }
  }
  return NS_OK;
}

nsresult
nsListControlFrame::DragMove(nsIDOMEvent* aMouseEvent)
{
  NS_ASSERTION(aMouseEvent, "aMouseEvent is null.");
  //REFLOW_DEBUG_MSG("DragMove\n");

  UpdateInListState(aMouseEvent);

  if (!IsInDropDownMode()) { 
    PRInt32 selectedIndex;
    if (NS_SUCCEEDED(GetIndexFromDOMEvent(aMouseEvent, selectedIndex))) {
      // Don't waste cycles if we already dragged over this item
      if (selectedIndex == mEndSelectionIndex) {
        return NS_OK;
      }
      nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aMouseEvent);
      NS_ASSERTION(mouseEvent, "aMouseEvent is not an nsIDOMMouseEvent!");
      PRBool isControl;
#if defined(XP_MAC) || defined(XP_MACOSX)
      mouseEvent->GetMetaKey(&isControl);
#else
      mouseEvent->GetCtrlKey(&isControl);
#endif
      // Turn SHIFT on when you are dragging, unless control is on.
      PRBool wasChanged = PerformSelection(selectedIndex,
                                           !isControl, isControl);
      mChangesSinceDragStart = mChangesSinceDragStart || wasChanged;
    }
  }
  return NS_OK;
}

//----------------------------------------------------------------------
// Scroll helpers.
//----------------------------------------------------------------------
nsresult
nsListControlFrame::ScrollToIndex(PRInt32 aIndex)
{
  if (aIndex < 0) {
    // XXX shouldn't we just do nothing if we're asked to scroll to
    // kNothingSelected?
    return ScrollToFrame(nsnull);
  } else {
    nsCOMPtr<nsIContent> content = GetOptionContent(aIndex);
    if (content) {
      return ScrollToFrame(content);
    }
  }

  return NS_ERROR_FAILURE;
}

nsresult
nsListControlFrame::ScrollToFrame(nsIContent* aOptElement)
{
  nsIScrollableView* scrollableView = GetScrollableView();

  if (scrollableView) {
    // if null is passed in we scroll to 0,0
    if (nsnull == aOptElement) {
      scrollableView->ScrollTo(0, 0, PR_TRUE);
      return NS_OK;
    }
  
    // otherwise we find the content's frame and scroll to it
    nsIPresShell *presShell = GetPresContext()->PresShell();
    nsIFrame * childframe;
    nsresult result;
    if (aOptElement) {
      result = presShell->GetPrimaryFrameFor(aOptElement, &childframe);
    } else {
      return NS_ERROR_FAILURE;
    }

    if (NS_SUCCEEDED(result) && childframe) {
      if (NS_SUCCEEDED(result) && scrollableView) {
        nscoord x;
        nscoord y;
        scrollableView->GetScrollPosition(x,y);
        // get the clipped rect
        nsRect rect = scrollableView->View()->GetBounds();
        // now move it by the offset of the scroll position
        rect.x = x;
        rect.y = y;

        // get the child
        nsRect fRect = childframe->GetRect();
        nsPoint pnt;
        nsIView * view;
        childframe->GetOffsetFromView(pnt, &view);

        // This change for 33421 (remove this comment later)

        // options can be a child of an optgroup
        // this checks to see the parent is an optgroup
        // and then adds in the parent's y coord
        // XXX this assume only one level of nesting of optgroups
        //   which is all the spec specifies at the moment.
        nsCOMPtr<nsIContent> parentContent = aOptElement->GetParent();
        nsCOMPtr<nsIDOMHTMLOptGroupElement> optGroup(do_QueryInterface(parentContent));
        nsRect optRect(0,0,0,0);
        if (optGroup) {
          nsIFrame * optFrame;
          result = presShell->GetPrimaryFrameFor(parentContent, &optFrame);
          if (NS_SUCCEEDED(result) && optFrame) {
            optRect = optFrame->GetRect();
          }
        }
        fRect.y += optRect.y;

        // See if the selected frame (fRect) is inside the scrolled
        // area (rect). Check only the vertical dimension. Don't
        // scroll just because there's horizontal overflow.
        if (!(rect.y <= fRect.y && fRect.YMost() <= rect.YMost())) {
          // figure out which direction we are going
          if (fRect.YMost() > rect.YMost()) {
            y = fRect.y-(rect.height-fRect.height);
          } else {
            y = fRect.y;
          }
          scrollableView->ScrollTo(pnt.x, y, PR_TRUE);
        }

      }
    }
  }
  return NS_OK;
}

//---------------------------------------------------------------------
// Ok, the entire idea of this routine is to move to the next item that 
// is suppose to be selected. If the item is disabled then we search in 
// the same direction looking for the next item to select. If we run off 
// the end of the list then we start at the end of the list and search 
// backwards until we get back to the original item or an enabled option
// 
// aStartIndex - the index to start searching from
// aNewIndex - will get set to the new index if it finds one
// aNumOptions - the total number of options in the list
// aDoAdjustInc - the initial increment 1-n
// aDoAdjustIncNext - the increment used to search for the next enabled option
//
// the aDoAdjustInc could be a "1" for a single item or
// any number greater representing a page of items
//
void
nsListControlFrame::AdjustIndexForDisabledOpt(PRInt32 aStartIndex,
                                              PRInt32 &aNewIndex,
                                              PRInt32 aNumOptions,
                                              PRInt32 aDoAdjustInc,
                                              PRInt32 aDoAdjustIncNext)
{
  // Cannot select anything if there is nothing to select
  if (aNumOptions == 0) {
    aNewIndex = kNothingSelected;
    return;
  }

  // means we reached the end of the list and now we are searching backwards
  PRBool doingReverse = PR_FALSE;
  // lowest index in the search range
  PRInt32 bottom      = 0;
  // highest index in the search range
  PRInt32 top         = aNumOptions;

  // Start off keyboard options at selectedIndex if nothing else is defaulted to
  //
  // XXX Perhaps this should happen for mouse too, to start off shift click
  // automatically in multiple ... to do this, we'd need to override
  // OnOptionSelected and set mStartSelectedIndex if nothing is selected.  Not
  // sure of the effects, though, so I'm not doing it just yet.
  PRInt32 startIndex = aStartIndex;
  if (startIndex < bottom) {
    GetSelectedIndex(&startIndex);
  }
  PRInt32 newIndex    = startIndex + aDoAdjustInc;

  // make sure we start off in the range
  if (newIndex < bottom) {
    newIndex = 0;
  } else if (newIndex >= top) {
    newIndex = aNumOptions-1;
  }

  while (1) {
    // if the newIndex isn't disabled, we are golden, bail out
    PRBool isDisabled = PR_TRUE;
    if (NS_SUCCEEDED(IsOptionDisabled(newIndex, isDisabled)) && !isDisabled) {
      break;
    }

    // it WAS disabled, so sart looking ahead for the next enabled option
    newIndex += aDoAdjustIncNext;

    // well, if we reach end reverse the search
    if (newIndex < bottom) {
      if (doingReverse) {
        return; // if we are in reverse mode and reach the end bail out
      } else {
        // reset the newIndex to the end of the list we hit
        // reverse the incrementer
        // set the other end of the list to our original starting index
        newIndex         = bottom;
        aDoAdjustIncNext = 1;
        doingReverse     = PR_TRUE;
        top              = startIndex;
      }
    } else  if (newIndex >= top) {
      if (doingReverse) {
        return;        // if we are in reverse mode and reach the end bail out
      } else {
        // reset the newIndex to the end of the list we hit
        // reverse the incrementer
        // set the other end of the list to our original starting index
        newIndex = top - 1;
        aDoAdjustIncNext = -1;
        doingReverse     = PR_TRUE;
        bottom           = startIndex;
      }
    }
  }

  // Looks like we found one
  aNewIndex     = newIndex;
}

nsAString& 
nsListControlFrame::GetIncrementalString()
{ 
  static nsString incrementalString;
  return incrementalString; 
}

void
nsListControlFrame::DropDownToggleKey(nsIDOMEvent* aKeyEvent)
{
  // Cocoa widgets do native popups, so don't try to show
  // dropdowns there.
  if (IsInDropDownMode() && !nsComboboxControlFrame::ToolkitHasNativePopup()) {
    PRBool isDroppedDown;
    mComboboxFrame->IsDroppedDown(&isDroppedDown);
    aKeyEvent->PreventDefault();
    nsIFrame* comboFrame;
    CallQueryInterface(mComboboxFrame, &comboFrame);
    nsWeakFrame weakFrame(comboFrame);
    mComboboxFrame->ShowDropDown(!isDroppedDown);
    if (!weakFrame.IsAlive())
      return;
    mComboboxFrame->RedisplaySelectedText();
  }
}

nsresult
nsListControlFrame::KeyPress(nsIDOMEvent* aKeyEvent)
{
  NS_ASSERTION(aKeyEvent, "keyEvent is null.");

  if (nsFormControlHelper::GetDisabled(mContent))
    return NS_OK;

  // Start by making sure we can query for a key event
  nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
  NS_ENSURE_TRUE(keyEvent, NS_ERROR_FAILURE);

  PRUint32 keycode = 0;
  PRUint32 charcode = 0;
  keyEvent->GetKeyCode(&keycode);
  keyEvent->GetCharCode(&charcode);
#ifdef DO_REFLOW_DEBUG
  if (code >= 32) {
    REFLOW_DEBUG_MSG3("KeyCode: %c %d\n", code, code);
  }
#endif

  PRBool isAlt = PR_FALSE;

  keyEvent->GetAltKey(&isAlt);
  if (isAlt) {
    if (keycode == nsIDOMKeyEvent::DOM_VK_UP || keycode == nsIDOMKeyEvent::DOM_VK_DOWN) {
      DropDownToggleKey(aKeyEvent);
    }
    return NS_OK;
  }

  // Get control / shift modifiers
  PRBool isControl = PR_FALSE;
  PRBool isShift   = PR_FALSE;
  keyEvent->GetCtrlKey(&isControl);
  if (!isControl) {
    keyEvent->GetMetaKey(&isControl);
  }
  keyEvent->GetShiftKey(&isShift);

  // now make sure there are options or we are wasting our time
  nsCOMPtr<nsIDOMHTMLOptionsCollection> options =
    getter_AddRefs(GetOptions(mContent));
  NS_ENSURE_TRUE(options, NS_ERROR_FAILURE);

  PRUint32 numOptions = 0;
  options->GetLength(&numOptions);

  // Whether we did an incremental search or another action
  PRBool didIncrementalSearch = PR_FALSE;
  
  // this is the new index to set
  // DOM_VK_RETURN & DOM_VK_ESCAPE will not set this
  PRInt32 newIndex = kNothingSelected;

  // set up the old and new selected index and process it
  // DOM_VK_RETURN selects the item
  // DOM_VK_ESCAPE cancels the selection
  // default processing checks to see if the pressed the first 
  //   letter of an item in the list and advances to it
  
  if (isControl && (keycode == nsIDOMKeyEvent::DOM_VK_UP ||
                    keycode == nsIDOMKeyEvent::DOM_VK_LEFT ||
                    keycode == nsIDOMKeyEvent::DOM_VK_DOWN ||
                    keycode == nsIDOMKeyEvent::DOM_VK_RIGHT)) {
    PRBool isMultiple;
    GetMultiple(&isMultiple);
    // Don't go into multiple select mode unless this list can handle it
    mControlSelectMode = isMultiple;
    isControl = isMultiple;
  } else if (charcode != ' ') {
    mControlSelectMode = PR_FALSE;
  }
  switch (keycode) {

    case nsIDOMKeyEvent::DOM_VK_UP:
    case nsIDOMKeyEvent::DOM_VK_LEFT: {
      REFLOW_DEBUG_MSG2("DOM_VK_UP   mEndSelectionIndex: %d ",
                        mEndSelectionIndex);
      AdjustIndexForDisabledOpt(mEndSelectionIndex, newIndex,
                                (PRInt32)numOptions,
                                -1, -1);
      REFLOW_DEBUG_MSG2("  After: %d\n", newIndex);
      } break;
    
    case nsIDOMKeyEvent::DOM_VK_DOWN:
    case nsIDOMKeyEvent::DOM_VK_RIGHT: {
      REFLOW_DEBUG_MSG2("DOM_VK_DOWN mEndSelectionIndex: %d ",
                        mEndSelectionIndex);

      AdjustIndexForDisabledOpt(mEndSelectionIndex, newIndex,
                                (PRInt32)numOptions,
                                1, 1);
      REFLOW_DEBUG_MSG2("  After: %d\n", newIndex);
      } break;

    case nsIDOMKeyEvent::DOM_VK_RETURN: {
      if (mComboboxFrame != nsnull) {
        PRBool droppedDown = PR_FALSE;
        mComboboxFrame->IsDroppedDown(&droppedDown);
        if (droppedDown) {
          nsWeakFrame weakFrame(this);
          ComboboxFinish(mEndSelectionIndex);
          if (!weakFrame.IsAlive())
            return NS_OK;
        }
        FireOnChange();
        return NS_OK;
      } else {
        newIndex = mEndSelectionIndex;
      }
      } break;

    case nsIDOMKeyEvent::DOM_VK_ESCAPE: {
      nsWeakFrame weakFrame(this);
      AboutToRollup();
      if (!weakFrame.IsAlive()) {
        aKeyEvent->PreventDefault(); // since we won't reach the one below
        return NS_OK;
      }
    } break;

    case nsIDOMKeyEvent::DOM_VK_PAGE_UP: {
      AdjustIndexForDisabledOpt(mEndSelectionIndex, newIndex,
                                (PRInt32)numOptions,
                                -(mNumDisplayRows-1), -1);
      } break;

    case nsIDOMKeyEvent::DOM_VK_PAGE_DOWN: {
      AdjustIndexForDisabledOpt(mEndSelectionIndex, newIndex,
                                (PRInt32)numOptions,
                                (mNumDisplayRows-1), 1);
      } break;

    case nsIDOMKeyEvent::DOM_VK_HOME: {
      AdjustIndexForDisabledOpt(0, newIndex,
                                (PRInt32)numOptions,
                                0, 1);
      } break;

    case nsIDOMKeyEvent::DOM_VK_END: {
      AdjustIndexForDisabledOpt(numOptions-1, newIndex,
                                (PRInt32)numOptions,
                                0, -1);
      } break;

#if defined(XP_WIN) || defined(XP_OS2)
    case nsIDOMKeyEvent::DOM_VK_F4: {
      DropDownToggleKey(aKeyEvent);
      return NS_OK;
    } break;
#endif

    case nsIDOMKeyEvent::DOM_VK_TAB: {
      return NS_OK;
    }

    default: { // Select option with this as the first character
               // XXX Not I18N compliant
      
      if (isControl && charcode != ' ') {
        return NS_OK;
      }

      didIncrementalSearch = PR_TRUE;
      if (charcode == 0) {
        // Backspace key will delete the last char in the string
        if (keycode == NS_VK_BACK && !GetIncrementalString().IsEmpty()) {
          GetIncrementalString().Truncate(GetIncrementalString().Length() - 1);
          aKeyEvent->PreventDefault();
        }
        return NS_OK;
      }
      
      DOMTimeStamp keyTime;
      aKeyEvent->GetTimeStamp(&keyTime);

      // Incremental Search: if time elapsed is below
      // INCREMENTAL_SEARCH_KEYPRESS_TIME, append this keystroke to the search
      // string we will use to find options and start searching at the current
      // keystroke.  Otherwise, Truncate the string if it's been a long time
      // since our last keypress.
      if (keyTime - gLastKeyTime > INCREMENTAL_SEARCH_KEYPRESS_TIME) {
        // If this is ' ' and we are at the beginning of the string, treat it as
        // "select this option" (bug 191543)
        if (charcode == ' ') {
          newIndex = mEndSelectionIndex;
          break;
        }
        GetIncrementalString().Truncate();
      }
      gLastKeyTime = keyTime;

      // Append this keystroke to the search string. 
      PRUnichar uniChar = ToLowerCase(NS_STATIC_CAST(PRUnichar, charcode));
      GetIncrementalString().Append(uniChar);

      // See bug 188199, if all letters in incremental string are same, just try to match the first one
      nsAutoString incrementalString(GetIncrementalString());
      PRUint32 charIndex = 1, stringLength = incrementalString.Length();
      while (charIndex < stringLength && incrementalString[charIndex] == incrementalString[charIndex - 1]) {
        charIndex++;
      }
      if (charIndex == stringLength) {
        incrementalString.Truncate(1);
        stringLength = 1;
      }

      // Determine where we're going to start reading the string
      // If we have multiple characters to look for, we start looking *at* the
      // current option.  If we have only one character to look for, we start
      // looking *after* the current option.	
      // Exception: if there is no option selected to start at, we always start
      // *at* 0.
      PRInt32 startIndex;
      GetSelectedIndex(&startIndex);
      if (startIndex == kNothingSelected) {
        startIndex = 0;
      } else if (stringLength == 1) {
        startIndex++;
      }

      PRUint32 i;
      for (i = 0; i < numOptions; i++) {
        PRUint32 index = (i + startIndex) % numOptions;
        nsCOMPtr<nsIDOMHTMLOptionElement> optionElement(getter_AddRefs(GetOption(options, index)));
        if (optionElement) {
          nsAutoString text;
          if (NS_OK == optionElement->GetText(text)) {
            if (StringBeginsWith(text, incrementalString,
                                 nsCaseInsensitiveStringComparator())) {
              PRBool wasChanged = PerformSelection(index, isShift, isControl);
              if (wasChanged) {
                // dispatch event, update combobox, etc.
                if (!UpdateSelection()) {
                  return NS_OK;
                }
              }
              break;
            }
          }
        }
      } // for

    } break;//case
  } // switch

  // We ate the key if we got this far.
  aKeyEvent->PreventDefault();

  // If we didn't do an incremental search, clear the string
  if (!didIncrementalSearch) {
    GetIncrementalString().Truncate();
  }

  // Actually process the new index and let the selection code
  // do the scrolling for us
  if (newIndex != kNothingSelected) {
    // If you hold control, no key will actually do anything except space.
    PRBool wasChanged = PR_FALSE;
    if (isControl && charcode != ' ') {
      mStartSelectionIndex = newIndex;
      mEndSelectionIndex = newIndex;
      ScrollToIndex(newIndex);
    } else if (mControlSelectMode && charcode == ' ') {
      wasChanged = SingleSelection(newIndex, PR_TRUE);
    } else {
      wasChanged = PerformSelection(newIndex, isShift, isControl);
    }
    if (wasChanged) {
       // dispatch event, update combobox, etc.
      if (!UpdateSelection()) {
        return NS_OK;
      }
    }
#ifdef ACCESSIBILITY
    if (charcode != ' ') {
      FireMenuItemActiveEvent();
    }
#endif

    // XXX - Are we cover up a problem here???
    // Why aren't they getting flushed each time?
    // because this isn't needed for Gfx
    if (IsInDropDownMode()) {
      // Don't flush anything but reflows lest it destroy us
      GetPresContext()->PresShell()->
        GetDocument()->FlushPendingNotifications(Flush_OnlyReflow);
    }
    REFLOW_DEBUG_MSG2("  After: %d\n", newIndex);

    // Make sure the SelectArea frame gets painted
    Invalidate(nsRect(0,0,mRect.width,mRect.height), PR_TRUE);

  } else {
    REFLOW_DEBUG_MSG("  After: SKIPPED it\n");
  }

  return NS_OK;
}


/******************************************************************************
 * nsListEventListener
 *****************************************************************************/

NS_IMPL_ADDREF(nsListEventListener)
NS_IMPL_RELEASE(nsListEventListener)
NS_INTERFACE_MAP_BEGIN(nsListEventListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseMotionListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener, nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMMouseListener)
NS_INTERFACE_MAP_END

#define FORWARD_EVENT(_event) \
NS_IMETHODIMP \
nsListEventListener::_event(nsIDOMEvent* aEvent) \
{ \
  if (mFrame) \
    return mFrame->nsListControlFrame::_event(aEvent); \
  return NS_OK; \
}

#define IGNORE_EVENT(_event) \
NS_IMETHODIMP \
nsListEventListener::_event(nsIDOMEvent* aEvent) \
{ return NS_OK; }

IGNORE_EVENT(HandleEvent)

/*================== nsIDOMKeyListener =========================*/

IGNORE_EVENT(KeyDown)
IGNORE_EVENT(KeyUp)
FORWARD_EVENT(KeyPress)

/*=============== nsIDOMMouseListener ======================*/

FORWARD_EVENT(MouseDown)
FORWARD_EVENT(MouseUp)
IGNORE_EVENT(MouseClick)
IGNORE_EVENT(MouseDblClick)
IGNORE_EVENT(MouseOver)
IGNORE_EVENT(MouseOut)

/*=============== nsIDOMMouseMotionListener ======================*/

FORWARD_EVENT(MouseMove)
// XXXbryner does anyone call this, ever?
IGNORE_EVENT(DragMove)

#undef FORWARD_EVENT
#undef IGNORE_EVENT

