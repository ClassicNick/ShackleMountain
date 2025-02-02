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
 *   Dave Hyatt <hyatt@mozilla.org> (Original Author)
 *   Dean Tessman <dean_tessman@hotmail.com>
 *   Brian Ryner <bryner@brianryner.com>
 *   Jan Varga <varga@ku.sk>
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

#include "nsLeafBoxFrame.h"
#include "nsPITreeBoxObject.h"
#include "nsITreeView.h"
#include "nsICSSPseudoComparator.h"
#include "nsIScrollbarMediator.h"
#include "nsIDragSession.h"
#include "nsITimer.h"
#include "nsIReflowCallback.h"
#include "nsILookAndFeel.h"
#include "nsValueArray.h"
#include "nsTreeStyleCache.h"
#include "nsTreeColumns.h"
#include "nsTreeImageListener.h"
#include "nsAutoPtr.h"
#include "nsDataHashtable.h"
#include "imgIRequest.h"
#include "imgIDecoderObserver.h"

class nsTreeBodyFrame;
class nsTreeReflowCallback : public nsIReflowCallback
{
public:
  nsTreeReflowCallback(nsTreeBodyFrame* aFrame) : mFrame(aFrame) {}
  NS_DECL_ISUPPORTS
  NS_IMETHOD ReflowFinished(nsIPresShell* aShell, PRBool* aFlushFlag);
  nsTreeBodyFrame* mFrame;
};

// An entry in the tree's image cache
struct nsTreeImageCacheEntry
{
  nsTreeImageCacheEntry() {}
  nsTreeImageCacheEntry(imgIRequest *aRequest, imgIDecoderObserver *aListener)
    : request(aRequest), listener(aListener) {}

  nsCOMPtr<imgIRequest> request;
  nsCOMPtr<imgIDecoderObserver> listener;
};

// The actual frame that paints the cells and rows.
class nsTreeBodyFrame : public nsLeafBoxFrame,
                        public nsITreeBoxObject,
                        public nsICSSPseudoComparator,
                        public nsIScrollbarMediator
{
public:
  nsTreeBodyFrame(nsIPresShell* aPresShell);
  virtual ~nsTreeBodyFrame();

  NS_DECL_ISUPPORTS
  NS_DECL_NSITREEBOXOBJECT

  // nsIBox
  NS_IMETHOD GetMinSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD SetBounds(nsBoxLayoutState& aBoxLayoutState, const nsRect& aRect,
                       PRBool aRemoveOverflowArea = PR_FALSE);

  nsresult ReflowFinished(nsIPresShell* aPresShell, PRBool* aFlushFlag);

  // nsICSSPseudoComparator
  NS_IMETHOD PseudoMatches(nsIAtom* aTag, nsCSSSelector* aSelector, PRBool* aResult);

  // nsIScrollbarMediator
  NS_IMETHOD PositionChanged(nsISupports* aScrollbar, PRInt32 aOldIndex, PRInt32& aNewIndex);
  NS_IMETHOD ScrollbarButtonPressed(nsISupports* aScrollbar, PRInt32 aOldIndex, PRInt32 aNewIndex);
  NS_IMETHOD VisibilityChanged(nsISupports* aScrollbar, PRBool aVisible) { Invalidate(); return NS_OK; };

  // Overridden from nsIFrame to cache our pres context.
  NS_IMETHOD Init(nsPresContext* aPresContext, nsIContent* aContent,
                  nsIFrame* aParent, nsStyleContext* aContext, nsIFrame* aPrevInFlow);
  NS_IMETHOD Destroy(nsPresContext* aPresContext);
  NS_IMETHOD GetCursor(const nsPoint& aPoint,
                       nsIFrame::Cursor& aCursor);

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext,
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

  // Painting methods.
  // Paint is the generic nsIFrame paint method.  We override this method
  // to paint our contents (our rows and cells).
  NS_IMETHOD Paint(nsPresContext*      aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect,
                   nsFramePaintLayer    aWhichLayer,
                   PRUint32             aFlags = 0);

  // This method paints a specific column background of the tree.
  void PaintColumn(nsTreeColumn*        aColumn,
                   const nsRect&        aColumnRect,
                   nsPresContext*      aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect);

  // This method paints a single row in the tree.
  void PaintRow(PRInt32              aRowIndex,
                const nsRect&        aRowRect,
                nsPresContext*      aPresContext,
                nsIRenderingContext& aRenderingContext,
                const nsRect&        aDirtyRect);

  // This method paints a single separator in the tree.
  void PaintSeparator(PRInt32              aRowIndex,
                      const nsRect&        aSeparatorRect,
                      nsPresContext*      aPresContext,
                      nsIRenderingContext& aRenderingContext,
                      const nsRect&        aDirtyRect);

  // This method paints a specific cell in a given row of the tree.
  void PaintCell(PRInt32              aRowIndex, 
                 nsTreeColumn*        aColumn,
                 const nsRect&        aCellRect,
                 nsPresContext*      aPresContext,
                 nsIRenderingContext& aRenderingContext,
                 const nsRect&        aDirtyRect,
                 nscoord&             aCurrX);

  // This method paints the twisty inside a cell in the primary column of an tree.
  void PaintTwisty(PRInt32              aRowIndex,
                   nsTreeColumn*        aColumn,
                   const nsRect&        aTwistyRect,
                   nsPresContext*      aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect,
                   nscoord&             aRemainingWidth,
                   nscoord&             aCurrX);

  // This method paints the image inside the cell of an tree.
  void PaintImage(PRInt32              aRowIndex,
                  nsTreeColumn*        aColumn,
                  const nsRect&        aImageRect,
                  nsPresContext*      aPresContext,
                  nsIRenderingContext& aRenderingContext,
                  const nsRect&        aDirtyRect,
                  nscoord&             aRemainingWidth,
                  nscoord&             aCurrX);

  // This method paints the text string inside a particular cell of the tree.
  void PaintText(PRInt32              aRowIndex, 
                 nsTreeColumn*        aColumn,
                 const nsRect&        aTextRect,
                 nsPresContext*      aPresContext,
                 nsIRenderingContext& aRenderingContext,
                 const nsRect&        aDirtyRect,
                 nscoord&             aCurrX);

  // This method paints the checkbox inside a particular cell of the tree.
  void PaintCheckbox(PRInt32              aRowIndex, 
                     nsTreeColumn*        aColumn,
                     const nsRect&        aCheckboxRect,
                     nsPresContext*      aPresContext,
                     nsIRenderingContext& aRenderingContext,
                     const nsRect&        aDirtyRect);

  // This method paints the progress meter inside a particular cell of the tree.
  void PaintProgressMeter(PRInt32              aRowIndex, 
                          nsTreeColumn*        aColumn,
                          const nsRect&        aProgressMeterRect,
                          nsPresContext*      aPresContext,
                          nsIRenderingContext& aRenderingContext,
                          const nsRect&        aDirtyRect);

  // This method paints a drop feedback of the tree.
  void PaintDropFeedback(const nsRect&        aDropFeedbackRect, 
                         nsPresContext*      aPresContext,
                         nsIRenderingContext& aRenderingContext,
                         const nsRect&        aDirtyRect);

  // This method is called with a specific style context and rect to
  // paint the background rect as if it were a full-blown frame.
  void PaintBackgroundLayer(nsStyleContext*      aStyleContext,
                            nsPresContext*      aPresContext, 
                            nsIRenderingContext& aRenderingContext, 
                            const nsRect&        aRect,
                            const nsRect&        aDirtyRect);

  friend nsresult NS_NewTreeBodyFrame(nsIPresShell* aPresShell, 
                                          nsIFrame** aNewFrame);

  struct ScrollParts {
    nsIScrollbarFrame* mVScrollbar;
    nsIContent*        mVScrollbarContent;
  };

protected:
  PRInt32 GetLastVisibleRow() {
    return mTopRowIndex + mPageLength;
  };

  // An internal hit test.  aX and aY are expected to be in twips in the
  // coordinate system of this frame.
  PRInt32 GetRowAt(nscoord aX, nscoord aY);

  // A helper used when hit testing.
  nsIAtom* GetItemWithinCellAt(nscoord aX, const nsRect& aCellRect,
                               PRInt32 aRowIndex, nsTreeColumn* aColumn);

  // An internal hit test.  aX and aY are expected to be in twips in the
  // coordinate system of this frame.
  void GetCellAt(nscoord aX, nscoord aY, PRInt32* aRow, nsTreeColumn** aCol,
                 nsIAtom** aChildElt);

  // Fetch an image from the image cache.
  nsresult GetImage(PRInt32 aRowIndex, nsTreeColumn* aCol, PRBool aUseContext,
                    nsStyleContext* aStyleContext, PRBool& aAllowImageRegions, imgIContainer** aResult);

  // Returns the size of a given image.   This size *includes* border and
  // padding.  It does not include margins.
  nsRect GetImageSize(PRInt32 aRowIndex, nsTreeColumn* aCol, PRBool aUseContext, nsStyleContext* aStyleContext);

  // Returns the height of rows in the tree.
  PRInt32 GetRowHeight();

  // Returns our indentation width.
  PRInt32 GetIndentation();

  // Calculates our width/height once border and padding have been removed.
  void CalcInnerBox();

  // Looks up a style context in the style cache.  On a cache miss we resolve
  // the pseudo-styles passed in and place them into the cache.
  nsStyleContext* GetPseudoStyleContext(nsIAtom* aPseudoElement);

  // Retrieves the scrollbars and scrollview relevant to this treebody. We
  // traverse the frame tree under our base element, in frame order, looking
  // for the first relevant vertical scrollbar, horizontal scrollbar, and
  // scrollable frame (with associated content and scrollable view). These
  // are all volatile and should not be retained.
  ScrollParts GetScrollParts();

  // Update the curpos of the scrollbar.
  void UpdateScrollbar(const ScrollParts& aParts);

  // Update the maxpos of the scrollbar.
  void InvalidateScrollbar(const ScrollParts& aParts);

  // Check vertical overflow.
  void CheckVerticalOverflow();
  
  // Calls UpdateScrollbar, Invalidate if aNeedsFullInvalidation is PR_TRUE,
  // InvalidateScrollbar and finally CheckVerticalOverflow.
  // Returns PR_TRUE if the frame is still alive after the method call.
  PRBool FullScrollbarUpdate(PRBool aNeedsFullInvalidation);

  // Use to auto-fill some of the common properties without the view having to do it.
  // Examples include container, open, selected, and focus.
  void PrefillPropertyArray(PRInt32 aRowIndex, nsTreeColumn* aCol);

  nsresult ScrollInternal(const ScrollParts& aParts, PRInt32 aRow);
  nsresult ScrollToRowInternal(const ScrollParts& aParts, PRInt32 aRow);
  nsresult EnsureRowIsVisibleInternal(const ScrollParts& aParts, PRInt32 aRow);

  // Convert client pixels into twips in our coordinate space.
  void AdjustClientCoordsToBoxCoordSpace(PRInt32 aX, PRInt32 aY,
                                         nscoord* aResultX, nscoord* aResultY);

  // Convert a border style into line style.
  nsLineStyle ConvertBorderStyleToLineStyle(PRUint8 aBorderStyle);

  // Cache the box object
  void EnsureBoxObject();

  void EnsureView();

  // Get the base element, <tree> or <select>
  nsIContent* GetBaseElement();

  void GetCellWidth(PRInt32 aRow, nsTreeColumn* aCol,
                    nsIRenderingContext* aRenderingContext,
                    nscoord& aDesiredSize, nscoord& aCurrentSize);
  nscoord CalcMaxRowWidth();

  PRBool CanAutoScroll(PRInt32 aRowIndex);

  // Calc the row and above/below/on status given where the mouse currently is hovering.
  // Also calc if we're in the region in which we want to auto-scroll the tree.
  // A positive value of |aScrollLines| means scroll down, a negative value
  // means scroll up, a zero value means that we aren't in drag scroll region.
  void ComputeDropPosition(nsGUIEvent* aEvent, PRInt32* aRow, PRInt16* aOrient,
                           PRInt16* aScrollLines);

  // Mark ourselves dirty if we're a select widget
  void MarkDirtyIfSelect();

  void InvalidateDropFeedback(PRInt32 aRow, PRInt16 aOrientation) {
    InvalidateRow(aRow);
    if (aOrientation != nsITreeView::DROP_ON)
      InvalidateRow(aRow + aOrientation);
  };

  // Create a new timer. This method is used to delay various actions like
  // opening/closing folders or tree scrolling.
  // aID is type of the action, aFunc is the function to be called when
  // the timer fires and aType is type of timer - one shot or repeating.
  nsresult CreateTimer(const nsILookAndFeel::nsMetricID aID,
                       nsTimerCallbackFunc aFunc, PRInt32 aType,
                       nsITimer** aTimer);

  static void OpenCallback(nsITimer *aTimer, void *aClosure);

  static void CloseCallback(nsITimer *aTimer, void *aClosure);

  static void LazyScrollCallback(nsITimer *aTimer, void *aClosure);

  static void ScrollCallback(nsITimer *aTimer, void *aClosure);

protected: // Data Members
  // Our cached pres context.
  nsPresContext* mPresContext;

  // The cached box object parent.
  nsCOMPtr<nsPITreeBoxObject> mTreeBoxObject;

  // Cached column information.
  nsRefPtr<nsTreeColumns> mColumns;

  // The current view for this tree widget.  We get all of our row and cell data
  // from the view.
  nsCOMPtr<nsITreeView> mView;    

  // A cache of all the style contexts we have seen for rows and cells of the tree.  This is a mapping from
  // a list of atoms to a corresponding style context.  This cache stores every combination that
  // occurs in the tree, so for n distinct properties, this cache could have 2 to the n entries
  // (the power set of all row properties).
  nsTreeStyleCache mStyleCache;

  // A hashtable that maps from URLs to image request/listener pairs.  The URL
  // is provided by the view or by the style context. The style context
  // represents a resolved :-moz-tree-cell-image (or twisty) pseudo-element.
  // It maps directly to an imgIRequest.
  nsDataHashtable<nsStringHashKey, nsTreeImageCacheEntry> mImageCache;

  // The index of the first visible row and the # of rows visible onscreen.  
  // The tree only examines onscreen rows, starting from
  // this index and going up to index+pageLength.
  PRInt32 mTopRowIndex;
  PRInt32 mPageLength;

  // Cached heights and indent info.
  nsRect mInnerBox;
  PRInt32 mRowHeight;
  PRInt32 mIndentation;
  nscoord mStringWidth;

  // A scratch array used when looking up cached style contexts.
  nsCOMPtr<nsISupportsArray> mScratchArray;

  // Whether or not we're currently focused.
  PRPackedBool mFocused;

  // Do we have a fixed number of onscreen rows?
  PRPackedBool mHasFixedRowCount;

  PRPackedBool mVerticalOverflow;

  nsRefPtr<nsTreeReflowCallback> mReflowCallback;

  PRInt32 mUpdateBatchNest;

  // Cached row count.
  PRInt32 mRowCount;

  class Slots {
    public:
      Slots()
        : mValueArray(~PRInt32(0)) {
      };

      ~Slots() {
        if (mTimer)
          mTimer->Cancel();
      };

      friend class nsTreeBodyFrame;

    protected:
      // If the drop is actually allowed here or not.
      PRBool                   mDropAllowed;

      // The row the mouse is hovering over during a drop.
      PRInt32                  mDropRow;

      // Where we want to draw feedback (above/on this row/below) if allowed.
      PRInt16                  mDropOrient;

      // Number of lines to be scrolled.
      PRInt16                  mScrollLines;

      nsCOMPtr<nsIDragSession> mDragSession;

      // Timer for opening/closing spring loaded folders or scrolling the tree.
      nsCOMPtr<nsITimer>       mTimer;

      // A value array used to keep track of all spring loaded folders.
      nsValueArray             mValueArray;
  };

  Slots* mSlots;
}; // class nsTreeBodyFrame
