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

#include "nsISupports.h"
#include "nsGUIEvent.h"
#include "nsDOMEvent.h"
#include "nsEventListenerManager.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMMouseMotionListener.h"
#include "nsIDOMContextMenuListener.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMFormListener.h"
#include "nsIDOMLoadListener.h"
#include "nsIDOMDragListener.h"
#include "nsIDOMPaintListener.h"
#include "nsIDOMTextListener.h"
#include "nsIDOMCompositionListener.h"
#include "nsIDOMXULListener.h"
#include "nsIDOMScrollListener.h"
#include "nsIDOMMutationListener.h"
#include "nsIDOMUIListener.h"
#include "nsIDOMPageTransitionListener.h"
#ifdef MOZ_SVG
#include "nsIDOMSVGListener.h"
#include "nsIDOMSVGZoomListener.h"
#include "nsSVGAtoms.h"
#endif // MOZ_SVG
#include "nsIEventStateManager.h"
#include "nsPIDOMWindow.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIJSEventListener.h"
#include "prmem.h"
#include "nsIScriptGlobalObject.h"
#include "nsLayoutAtoms.h"
#include "nsLayoutUtils.h"
#ifdef MOZ_XUL
// XXXbz the fact that this is ifdef MOZ_XUL is good indication that
// it doesn't belong here...
#include "nsITreeBoxObject.h"
#include "nsITreeColumns.h"
#include "nsIDOMXULMultSelectCntrlEl.h"
#endif
#include "nsINameSpaceManager.h"
#include "nsIContent.h"
#include "nsINodeInfo.h"
#include "nsIFrame.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsIScrollableView.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIScriptSecurityManager.h"
#include "nsDOMError.h"
#include "nsIJSContextStack.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsMutationEvent.h"
#include "nsIXPConnect.h"
#include "nsDOMCID.h"
#include "nsIScriptObjectOwner.h" // for nsIScriptEventHandlerOwner
#include "nsIClassInfo.h"
#include "nsIFocusController.h"
#include "nsIDOMElement.h"
#include "nsIBoxObject.h"
#include "nsIDOMNSDocument.h"
#include "nsIWidget.h"
#include "nsContentUtils.h"
#include "nsJSUtils.h"
#include "nsIDOMEventGroup.h"
#include "nsContentCID.h"

static NS_DEFINE_CID(kDOMScriptObjectFactoryCID,
                     NS_DOM_SCRIPT_OBJECT_FACTORY_CID);
static NS_DEFINE_CID(kDOMEventGroupCID, NS_DOMEVENTGROUP_CID);

typedef
NS_STDCALL_FUNCPROTO(nsresult,
                     GenericHandler,
                     nsIDOMEventListener, HandleEvent, 
                     (nsIDOMEvent*));

/*
 * Things here are not as they appear.  Namely, |ifaceListener| below is
 * not really a pointer to the nsIDOMEventListener interface, and aMethod is
 * not really a pointer-to-member for nsIDOMEventListener.  They both
 * actually refer to the event-type-specific listener interface.  The casting
 * magic allows us to use a single dispatch method.  This relies on the
 * assumption that nsIDOMEventListener and the event type listener interfaces
 * have the same object layout and will therefore have compatible
 * pointer-to-member implementations.
 */

static nsresult DispatchToInterface(nsIDOMEvent* aEvent,
                                    nsIDOMEventListener* aListener,
                                    GenericHandler aMethod,
                                    const nsIID& aIID,
                                    PRBool* aHasInterface)
{
  nsIDOMEventListener* ifaceListener = nsnull;
  nsresult rv = NS_OK;
  aListener->QueryInterface(aIID, (void**) &ifaceListener);
  if (ifaceListener) {
    *aHasInterface = PR_TRUE;
    rv = (ifaceListener->*aMethod)(aEvent);
    NS_RELEASE(ifaceListener);
  }
  return rv;
}

struct EventDispatchData
{
  PRUint32 message;
  GenericHandler method;
  PRUint8 bits;
};

struct EventTypeData
{
  const EventDispatchData* events;
  int                      numEvents;
  const nsIID*             iid;
};

#define HANDLER(x) NS_REINTERPRET_CAST(GenericHandler, x)

static const EventDispatchData sMouseEvents[] = {
  { NS_MOUSE_LEFT_BUTTON_DOWN,   HANDLER(&nsIDOMMouseListener::MouseDown),
    NS_EVENT_BITS_MOUSE_MOUSEDOWN },
  { NS_MOUSE_MIDDLE_BUTTON_DOWN, HANDLER(&nsIDOMMouseListener::MouseDown),
    NS_EVENT_BITS_MOUSE_MOUSEDOWN },
  { NS_MOUSE_RIGHT_BUTTON_DOWN,  HANDLER(&nsIDOMMouseListener::MouseDown),
    NS_EVENT_BITS_MOUSE_MOUSEDOWN },
  { NS_MOUSE_LEFT_BUTTON_UP,     HANDLER(&nsIDOMMouseListener::MouseUp),
    NS_EVENT_BITS_MOUSE_MOUSEUP },
  { NS_MOUSE_MIDDLE_BUTTON_UP,   HANDLER(&nsIDOMMouseListener::MouseUp),
    NS_EVENT_BITS_MOUSE_MOUSEUP },
  { NS_MOUSE_RIGHT_BUTTON_UP,    HANDLER(&nsIDOMMouseListener::MouseUp),
    NS_EVENT_BITS_MOUSE_MOUSEUP },
  { NS_MOUSE_LEFT_CLICK,         HANDLER(&nsIDOMMouseListener::MouseClick),
    NS_EVENT_BITS_MOUSE_CLICK },
  { NS_MOUSE_MIDDLE_CLICK,       HANDLER(&nsIDOMMouseListener::MouseClick),
    NS_EVENT_BITS_MOUSE_CLICK },
  { NS_MOUSE_RIGHT_CLICK,        HANDLER(&nsIDOMMouseListener::MouseClick),
    NS_EVENT_BITS_MOUSE_CLICK },
  { NS_MOUSE_LEFT_DOUBLECLICK,   HANDLER(&nsIDOMMouseListener::MouseDblClick),
    NS_EVENT_BITS_MOUSE_DBLCLICK },
  { NS_MOUSE_MIDDLE_DOUBLECLICK, HANDLER(&nsIDOMMouseListener::MouseDblClick),
    NS_EVENT_BITS_MOUSE_DBLCLICK },
  { NS_MOUSE_RIGHT_DOUBLECLICK,  HANDLER(&nsIDOMMouseListener::MouseDblClick),
    NS_EVENT_BITS_MOUSE_DBLCLICK },
  { NS_MOUSE_ENTER_SYNTH,        HANDLER(&nsIDOMMouseListener::MouseOver),
    NS_EVENT_BITS_MOUSE_MOUSEOVER },
  { NS_MOUSE_EXIT_SYNTH,         HANDLER(&nsIDOMMouseListener::MouseOut),
    NS_EVENT_BITS_MOUSE_MOUSEOUT }
};

static const EventDispatchData sMouseMotionEvents[] = {
  { NS_MOUSE_MOVE, HANDLER(&nsIDOMMouseMotionListener::MouseMove),
    NS_EVENT_BITS_MOUSEMOTION_MOUSEMOVE }
};

static const EventDispatchData sContextMenuEvents[] = {
  { NS_CONTEXTMENU,     HANDLER(&nsIDOMContextMenuListener::ContextMenu),
    NS_EVENT_BITS_CONTEXT_MENU },
  { NS_CONTEXTMENU_KEY, HANDLER(&nsIDOMContextMenuListener::ContextMenu),
    NS_EVENT_BITS_CONTEXT_MENU }
};

static const EventDispatchData sCompositionEvents[] = {
  { NS_COMPOSITION_START,  HANDLER(&nsIDOMCompositionListener::HandleStartComposition),
    NS_EVENT_BITS_COMPOSITION_START },
  { NS_COMPOSITION_END,    HANDLER(&nsIDOMCompositionListener::HandleEndComposition),
    NS_EVENT_BITS_COMPOSITION_END },
  { NS_COMPOSITION_QUERY,  HANDLER(&nsIDOMCompositionListener::HandleQueryComposition),
    NS_EVENT_BITS_COMPOSITION_QUERY },
  { NS_RECONVERSION_QUERY, HANDLER(&nsIDOMCompositionListener::HandleQueryReconversion),
    NS_EVENT_BITS_COMPOSITION_RECONVERSION },
  { NS_QUERYCARETRECT,  HANDLER(&nsIDOMCompositionListener::HandleQueryCaretRect),
    NS_EVENT_BITS_COMPOSITION_QUERYCARETRECT }
};

static const EventDispatchData sTextEvents[] = {
  {NS_TEXT_TEXT,HANDLER(&nsIDOMTextListener::HandleText),NS_EVENT_BITS_TEXT_TEXT},
};

static const EventDispatchData sKeyEvents[] = {
  {NS_KEY_UP,   HANDLER(&nsIDOMKeyListener::KeyUp),   NS_EVENT_BITS_KEY_KEYUP},
  {NS_KEY_DOWN, HANDLER(&nsIDOMKeyListener::KeyDown), NS_EVENT_BITS_KEY_KEYDOWN},
  {NS_KEY_PRESS,HANDLER(&nsIDOMKeyListener::KeyPress),NS_EVENT_BITS_KEY_KEYPRESS},
};

static const EventDispatchData sFocusEvents[] = {
  {NS_FOCUS_CONTENT,HANDLER(&nsIDOMFocusListener::Focus),NS_EVENT_BITS_FOCUS_FOCUS},
  {NS_BLUR_CONTENT, HANDLER(&nsIDOMFocusListener::Blur), NS_EVENT_BITS_FOCUS_BLUR }
};

static const EventDispatchData sFormEvents[] = {
  {NS_FORM_SUBMIT,  HANDLER(&nsIDOMFormListener::Submit),NS_EVENT_BITS_FORM_SUBMIT},
  {NS_FORM_RESET,   HANDLER(&nsIDOMFormListener::Reset), NS_EVENT_BITS_FORM_RESET},
  {NS_FORM_CHANGE,  HANDLER(&nsIDOMFormListener::Change),NS_EVENT_BITS_FORM_CHANGE},
  {NS_FORM_SELECTED,HANDLER(&nsIDOMFormListener::Select),NS_EVENT_BITS_FORM_SELECT},
  {NS_FORM_INPUT,   HANDLER(&nsIDOMFormListener::Input), NS_EVENT_BITS_FORM_INPUT}
};

static const EventDispatchData sLoadEvents[] = {
  {NS_PAGE_LOAD,   HANDLER(&nsIDOMLoadListener::Load),  NS_EVENT_BITS_LOAD_LOAD},
  {NS_IMAGE_LOAD,  HANDLER(&nsIDOMLoadListener::Load),  NS_EVENT_BITS_LOAD_LOAD},
  {NS_SCRIPT_LOAD, HANDLER(&nsIDOMLoadListener::Load),  NS_EVENT_BITS_LOAD_LOAD},
  {NS_PAGE_UNLOAD, HANDLER(&nsIDOMLoadListener::Unload),NS_EVENT_BITS_LOAD_UNLOAD},
  {NS_IMAGE_ERROR, HANDLER(&nsIDOMLoadListener::Error), NS_EVENT_BITS_LOAD_ERROR},
  {NS_SCRIPT_ERROR,HANDLER(&nsIDOMLoadListener::Error), NS_EVENT_BITS_LOAD_ERROR},
  {NS_BEFORE_PAGE_UNLOAD,HANDLER(&nsIDOMLoadListener::BeforeUnload), NS_EVENT_BITS_LOAD_BEFORE_UNLOAD},
};

static const EventDispatchData sPaintEvents[] = {
  {NS_PAINT,       HANDLER(&nsIDOMPaintListener::Paint), NS_EVENT_BITS_PAINT_PAINT},
  {NS_RESIZE_EVENT,HANDLER(&nsIDOMPaintListener::Resize),NS_EVENT_BITS_PAINT_RESIZE},
  {NS_SCROLL_EVENT,HANDLER(&nsIDOMPaintListener::Scroll),NS_EVENT_BITS_PAINT_SCROLL}
};

static const EventDispatchData sDragEvents[] = {
  {NS_DRAGDROP_ENTER,     HANDLER(&nsIDOMDragListener::DragEnter),  NS_EVENT_BITS_DRAG_ENTER},
  {NS_DRAGDROP_OVER_SYNTH,HANDLER(&nsIDOMDragListener::DragOver),   NS_EVENT_BITS_DRAG_OVER},
  {NS_DRAGDROP_EXIT_SYNTH,HANDLER(&nsIDOMDragListener::DragExit),   NS_EVENT_BITS_DRAG_EXIT},
  {NS_DRAGDROP_DROP,      HANDLER(&nsIDOMDragListener::DragDrop),   NS_EVENT_BITS_DRAG_DROP},
  {NS_DRAGDROP_GESTURE,   HANDLER(&nsIDOMDragListener::DragGesture),NS_EVENT_BITS_DRAG_GESTURE}
};

static const EventDispatchData sScrollEvents[] = {
  { NS_SCROLLPORT_OVERFLOW,        HANDLER(&nsIDOMScrollListener::Overflow),
    NS_EVENT_BITS_SCROLLPORT_OVERFLOW },
  { NS_SCROLLPORT_UNDERFLOW,       HANDLER(&nsIDOMScrollListener::Underflow),
    NS_EVENT_BITS_SCROLLPORT_UNDERFLOW },
  { NS_SCROLLPORT_OVERFLOWCHANGED, HANDLER(&nsIDOMScrollListener::OverflowChanged),
    NS_EVENT_BITS_SCROLLPORT_OVERFLOWCHANGED }
};

static const EventDispatchData sXULEvents[] = {
  { NS_XUL_POPUP_SHOWING,  HANDLER(&nsIDOMXULListener::PopupShowing),
    NS_EVENT_BITS_XUL_POPUP_SHOWING },
  { NS_XUL_POPUP_SHOWN,    HANDLER(&nsIDOMXULListener::PopupShown),
    NS_EVENT_BITS_XUL_POPUP_SHOWN },
  { NS_XUL_POPUP_HIDING,   HANDLER(&nsIDOMXULListener::PopupHiding),
    NS_EVENT_BITS_XUL_POPUP_HIDING },
  { NS_XUL_POPUP_HIDDEN,   HANDLER(&nsIDOMXULListener::PopupHidden),
    NS_EVENT_BITS_XUL_POPUP_HIDDEN },
  { NS_XUL_CLOSE,          HANDLER(&nsIDOMXULListener::Close),
    NS_EVENT_BITS_XUL_CLOSE },
  { NS_XUL_COMMAND,        HANDLER(&nsIDOMXULListener::Command),
    NS_EVENT_BITS_XUL_COMMAND },
  { NS_XUL_BROADCAST,      HANDLER(&nsIDOMXULListener::Broadcast),
    NS_EVENT_BITS_XUL_BROADCAST },
  { NS_XUL_COMMAND_UPDATE, HANDLER(&nsIDOMXULListener::CommandUpdate),
    NS_EVENT_BITS_XUL_COMMAND_UPDATE }
};

static const EventDispatchData sMutationEvents[] = {
  { NS_MUTATION_SUBTREEMODIFIED,
    HANDLER(&nsIDOMMutationListener::SubtreeModified),
    NS_EVENT_BITS_MUTATION_SUBTREEMODIFIED },
  { NS_MUTATION_NODEINSERTED,
    HANDLER(&nsIDOMMutationListener::NodeInserted),
    NS_EVENT_BITS_MUTATION_NODEINSERTED },
  { NS_MUTATION_NODEREMOVED,
    HANDLER(&nsIDOMMutationListener::NodeRemoved),
    NS_EVENT_BITS_MUTATION_NODEREMOVED },
  { NS_MUTATION_NODEINSERTEDINTODOCUMENT,
    HANDLER(&nsIDOMMutationListener::NodeInsertedIntoDocument),
    NS_EVENT_BITS_MUTATION_NODEINSERTEDINTODOCUMENT },
  { NS_MUTATION_NODEREMOVEDFROMDOCUMENT,
    HANDLER(&nsIDOMMutationListener::NodeRemovedFromDocument),
    NS_EVENT_BITS_MUTATION_NODEREMOVEDFROMDOCUMENT },
  { NS_MUTATION_ATTRMODIFIED,
    HANDLER(&nsIDOMMutationListener::AttrModified),
    NS_EVENT_BITS_MUTATION_ATTRMODIFIED },
  { NS_MUTATION_CHARACTERDATAMODIFIED,
    HANDLER(&nsIDOMMutationListener::CharacterDataModified),
    NS_EVENT_BITS_MUTATION_CHARACTERDATAMODIFIED }
};

static const EventDispatchData sUIEvents[] = {
  { NS_UI_ACTIVATE, HANDLER(&nsIDOMUIListener::Activate),
    NS_EVENT_BITS_UI_ACTIVATE },
  { NS_UI_FOCUSIN, HANDLER(&nsIDOMUIListener::FocusIn),
    NS_EVENT_BITS_UI_FOCUSIN },
  { NS_UI_FOCUSOUT, HANDLER(&nsIDOMUIListener::FocusOut),
    NS_EVENT_BITS_UI_FOCUSOUT }
};

static const EventDispatchData sPageTransitionEvents[] = {
  { NS_PAGE_SHOW, HANDLER(&nsIDOMPageTransitionListener::PageShow),
    NS_EVENT_BITS_PAGETRANSITION_SHOW },
  { NS_PAGE_HIDE, HANDLER(&nsIDOMPageTransitionListener::PageHide),
    NS_EVENT_BITS_PAGETRANSITION_HIDE }
};

#ifdef MOZ_SVG
static const EventDispatchData sSVGEvents[] = {
  { NS_SVG_LOAD, HANDLER(&nsIDOMSVGListener::Load),
    NS_EVENT_BITS_SVG_LOAD },
  { NS_SVG_UNLOAD, HANDLER(&nsIDOMSVGListener::Unload),
    NS_EVENT_BITS_SVG_UNLOAD },
  { NS_SVG_ABORT, HANDLER(&nsIDOMSVGListener::Abort),
    NS_EVENT_BITS_SVG_ABORT },
  { NS_SVG_ERROR, HANDLER(&nsIDOMSVGListener::Error),
    NS_EVENT_BITS_SVG_ERROR },
  { NS_SVG_RESIZE, HANDLER(&nsIDOMSVGListener::Resize),
    NS_EVENT_BITS_SVG_RESIZE },
  { NS_SVG_SCROLL, HANDLER(&nsIDOMSVGListener::Scroll),
    NS_EVENT_BITS_SVG_SCROLL }
};

static const EventDispatchData sSVGZoomEvents[] = {
  { NS_SVG_ZOOM, HANDLER(&nsIDOMSVGZoomListener::Zoom),
    NS_EVENT_BITS_SVGZOOM_ZOOM }
};
#endif // MOZ_SVG

#define IMPL_EVENTTYPEDATA(type) \
{ \
  s##type##Events, \
  NS_ARRAY_LENGTH(s##type##Events), \
  &NS_GET_IID(nsIDOM##type##Listener) \
}
 
// IMPORTANT: indices match up with eEventArrayType_ enum values

static const EventTypeData sEventTypes[] = {
  IMPL_EVENTTYPEDATA(Mouse),
  IMPL_EVENTTYPEDATA(MouseMotion),
  IMPL_EVENTTYPEDATA(ContextMenu),
  IMPL_EVENTTYPEDATA(Key),
  IMPL_EVENTTYPEDATA(Load),
  IMPL_EVENTTYPEDATA(Focus),
  IMPL_EVENTTYPEDATA(Form),
  IMPL_EVENTTYPEDATA(Drag),
  IMPL_EVENTTYPEDATA(Paint),
  IMPL_EVENTTYPEDATA(Text),
  IMPL_EVENTTYPEDATA(Composition),
  IMPL_EVENTTYPEDATA(XUL),
  IMPL_EVENTTYPEDATA(Scroll),
  IMPL_EVENTTYPEDATA(Mutation),
  IMPL_EVENTTYPEDATA(UI),
  IMPL_EVENTTYPEDATA(PageTransition)
#ifdef MOZ_SVG
 ,
  IMPL_EVENTTYPEDATA(SVG),
  IMPL_EVENTTYPEDATA(SVGZoom)
#endif // MOZ_SVG
};

// Strong references to event groups
nsIDOMEventGroup* gSystemEventGroup;
nsIDOMEventGroup* gDOM2EventGroup;

PRUint32 nsEventListenerManager::mInstanceCount = 0;

nsEventListenerManager::nsEventListenerManager() :
  mManagerType(NS_ELM_NONE),
  mListenersRemoved(PR_FALSE),
  mSingleListenerType(eEventArrayType_None),
  mSingleListener(nsnull),
  mMultiListeners(nsnull),
  mGenericListeners(nsnull),
  mTarget(nsnull)
{
  ++mInstanceCount;
}

static PRBool PR_CALLBACK
GenericListenersHashEnum(nsHashKey *aKey, void *aData, void* closure)
{
  nsVoidArray* listeners = NS_STATIC_CAST(nsVoidArray*, aData);
  if (listeners) {
    PRInt32 i, count = listeners->Count();
    nsListenerStruct *ls;
    PRBool* scriptOnly = NS_STATIC_CAST(PRBool*, closure);
    for (i = count-1; i >= 0; --i) {
      ls = (nsListenerStruct*)listeners->ElementAt(i);
      if (ls) {
        if (*scriptOnly) {
          if (ls->mFlags & NS_PRIV_EVENT_FLAG_SCRIPT) {
            NS_RELEASE(ls->mListener);
            //listeners->RemoveElement((void*)ls); we delete the entire array anyways, no need to RemoveElement
            PR_DELETE(ls);
          }
        }
        else {
          NS_IF_RELEASE(ls->mListener);
          PR_DELETE(ls);
        }
      }
    }
    //Only delete if we were removing all listeners
    if (!*scriptOnly) {
      delete listeners;
    }
  }
  return PR_TRUE;
}

nsEventListenerManager::~nsEventListenerManager() 
{
  RemoveAllListeners(PR_FALSE);

  --mInstanceCount;
  if(mInstanceCount == 0) {
    NS_IF_RELEASE(gSystemEventGroup);
    NS_IF_RELEASE(gDOM2EventGroup);
  }
}

nsresult
nsEventListenerManager::RemoveAllListeners(PRBool aScriptOnly)
{
  if (!aScriptOnly) {
    mListenersRemoved = PR_TRUE;
  }

  ReleaseListeners(&mSingleListener, aScriptOnly);
  if (!mSingleListener) {
    mSingleListenerType = eEventArrayType_None;
    mManagerType &= ~NS_ELM_SINGLE;
  }

  if (mMultiListeners) {
    // XXX probably should just be i < Count()
    for (PRInt32 i=0; i<EVENT_ARRAY_TYPE_LENGTH && i < mMultiListeners->Count(); i++) {
      nsVoidArray* listeners;
      listeners = NS_STATIC_CAST(nsVoidArray*, mMultiListeners->ElementAt(i));
      ReleaseListeners(&listeners, aScriptOnly);
    }
    if (!aScriptOnly) {
      delete mMultiListeners;
      mMultiListeners = nsnull;
      mManagerType &= ~NS_ELM_MULTI;
    }
  }

  if (mGenericListeners) {
    PRBool scriptOnly = aScriptOnly;
    mGenericListeners->Enumerate(GenericListenersHashEnum, &scriptOnly);
    //hash destructor
    if (!aScriptOnly) {
      delete mGenericListeners;
      mGenericListeners = nsnull;
      mManagerType &= ~NS_ELM_HASH;
    }
  }

  return NS_OK;
}

void
nsEventListenerManager::Shutdown()
{
    sAddListenerID = JSVAL_VOID;

    nsDOMEvent::Shutdown();
}

NS_IMPL_ADDREF(nsEventListenerManager)
NS_IMPL_RELEASE(nsEventListenerManager)


NS_INTERFACE_MAP_BEGIN(nsEventListenerManager)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIEventListenerManager)
   NS_INTERFACE_MAP_ENTRY(nsIEventListenerManager)
   NS_INTERFACE_MAP_ENTRY(nsIDOMEventTarget)
   NS_INTERFACE_MAP_ENTRY(nsIDOM3EventTarget)
   NS_INTERFACE_MAP_ENTRY(nsIDOMEventReceiver)
NS_INTERFACE_MAP_END


nsVoidArray*
nsEventListenerManager::GetListenersByType(EventArrayType aType, 
                                           nsHashKey* aKey, PRBool aCreate)
{
  NS_ASSERTION(aType >= 0,"Negative EventListenerType?");
  //Look for existing listeners
  if (aType == eEventArrayType_Hash && aKey && (mManagerType & NS_ELM_HASH)) {
    if (mGenericListeners && mGenericListeners->Exists(aKey)) {
      nsVoidArray* listeners = NS_STATIC_CAST(nsVoidArray*, mGenericListeners->Get(aKey));
      return listeners;
    }
  }
  else if (mManagerType & NS_ELM_SINGLE) {
    if (mSingleListenerType == aType) {
      return mSingleListener;
    }
  }
  else if (mManagerType & NS_ELM_MULTI) {
    if (mMultiListeners) {
      PRInt32 index = aType;
      if (index < mMultiListeners->Count()) {
        nsVoidArray* listeners;
        listeners = NS_STATIC_CAST(nsVoidArray*, mMultiListeners->ElementAt(index));
        if (listeners) {
          return listeners;
        }
      }
    }
  }

  //If we've gotten here we didn't find anything.  See if we should create something.
  if (aCreate) {
    if (aType == eEventArrayType_Hash && aKey) {
      if (!mGenericListeners) {
        mGenericListeners = new nsHashtable();
        if (!mGenericListeners) {
          //out of memory
          return nsnull;
        }
      }
      NS_ASSERTION(!(mGenericListeners->Get(aKey)), "Found existing generic listeners, should be none");
      nsVoidArray* listeners;
      listeners = new nsAutoVoidArray();
      if (!listeners) {
        //out of memory
        return nsnull;
      }
      mGenericListeners->Put(aKey, listeners);
      mManagerType |= NS_ELM_HASH;
      return listeners;
    }
    else {
      if (mManagerType & NS_ELM_SINGLE) {
        //Change single type into multi, then add new listener with the code for the 
        //multi type below
        NS_ASSERTION(!mMultiListeners, "Found existing multi listener array, should be none");
        mMultiListeners = new nsAutoVoidArray();
        if (!mMultiListeners) {
          //out of memory
          return nsnull;
        }

        //Move single listener to multi array
        mMultiListeners->ReplaceElementAt((void*)mSingleListener, mSingleListenerType);
        mSingleListener = nsnull;

        mManagerType &= ~NS_ELM_SINGLE;
        mManagerType |= NS_ELM_MULTI;
        // we'll fall through into the multi case
      }

      if (mManagerType & NS_ELM_MULTI) {
        PRInt32 index = aType;
        if (index >= 0) {
          nsVoidArray* listeners;
          NS_ASSERTION(index >= mMultiListeners->Count() || !mMultiListeners->ElementAt(index), "Found existing listeners, should be none");
          listeners = new nsAutoVoidArray();
          if (!listeners) {
            //out of memory
            return nsnull;
          }
          mMultiListeners->ReplaceElementAt((void*)listeners, index);
          return listeners;
        }
      }
      else {
        //We had no pre-existing type.  This is our first non-hash listener.
        //Create the single listener type
        NS_ASSERTION(!mSingleListener, "Found existing single listener array, should be none");
        mSingleListener = new nsAutoVoidArray();
        if (!mSingleListener) {
          //out of memory
          return nsnull;
        }
        mSingleListenerType = aType;
        mManagerType |= NS_ELM_SINGLE;
        return mSingleListener;
      }
    }
  }

  return nsnull;
}

EventArrayType
nsEventListenerManager::GetTypeForIID(const nsIID& aIID)
{ 
  if (aIID.Equals(NS_GET_IID(nsIDOMMouseListener)))
      return eEventArrayType_Mouse;

  if (aIID.Equals(NS_GET_IID(nsIDOMMouseMotionListener)))
    return eEventArrayType_MouseMotion;

  if (aIID.Equals(NS_GET_IID(nsIDOMContextMenuListener)))
    return eEventArrayType_ContextMenu;

  if (aIID.Equals(NS_GET_IID(nsIDOMKeyListener)))
    return eEventArrayType_Key;

  if (aIID.Equals(NS_GET_IID(nsIDOMLoadListener)))
    return eEventArrayType_Load;

  if (aIID.Equals(NS_GET_IID(nsIDOMFocusListener)))
    return eEventArrayType_Focus;

  if (aIID.Equals(NS_GET_IID(nsIDOMFormListener)))
    return eEventArrayType_Form;

  if (aIID.Equals(NS_GET_IID(nsIDOMDragListener)))
    return eEventArrayType_Drag;

  if (aIID.Equals(NS_GET_IID(nsIDOMPaintListener)))
    return eEventArrayType_Paint;

  if (aIID.Equals(NS_GET_IID(nsIDOMTextListener)))
    return eEventArrayType_Text;

  if (aIID.Equals(NS_GET_IID(nsIDOMCompositionListener)))
    return eEventArrayType_Composition;

  if (aIID.Equals(NS_GET_IID(nsIDOMXULListener)))
    return eEventArrayType_XUL;

  if (aIID.Equals(NS_GET_IID(nsIDOMScrollListener)))
    return eEventArrayType_Scroll;

  if (aIID.Equals(NS_GET_IID(nsIDOMMutationListener)))
    return eEventArrayType_Mutation;

  if (aIID.Equals(NS_GET_IID(nsIDOMUIListener)))
    return eEventArrayType_DOMUI;

#ifdef MOZ_SVG
  if (aIID.Equals(NS_GET_IID(nsIDOMSVGListener)))
    return eEventArrayType_SVG;

  if (aIID.Equals(NS_GET_IID(nsIDOMSVGZoomListener)))
    return eEventArrayType_SVGZoom;
#endif // MOZ_SVG

  return eEventArrayType_None;
}

void
nsEventListenerManager::ReleaseListeners(nsVoidArray** aListeners,
                                         PRBool aScriptOnly)
{
  if (nsnull != *aListeners) {
    PRInt32 i, count = (*aListeners)->Count();
    nsListenerStruct *ls;
    for (i = 0; i < count; i++) {
      ls = (nsListenerStruct*)(*aListeners)->ElementAt(i);
      if (ls) {
        if (aScriptOnly) {
          if (ls->mFlags & NS_PRIV_EVENT_FLAG_SCRIPT) {
            NS_RELEASE(ls->mListener);
            //(*aListeners)->RemoveElement((void*)ls); We're going to delete the array anyways
            PR_DELETE(ls);
          }
        }
        else {
          NS_IF_RELEASE(ls->mListener);
          PR_DELETE(ls);
        }
      }
    }
    //Only delete if we were removing all listeners
    if (!aScriptOnly) {
      delete *aListeners;
      *aListeners = nsnull;
    }
  }
}

/**
* Sets events listeners of all types. 
* @param an event listener
*/

nsresult
nsEventListenerManager::AddEventListener(nsIDOMEventListener *aListener, 
                                         EventArrayType aType, 
                                         PRInt32 aSubType,
                                         nsHashKey* aKey,
                                         PRInt32 aFlags,
                                         nsIDOMEventGroup* aEvtGrp)
{
  NS_ENSURE_TRUE(aListener, NS_ERROR_FAILURE);

  nsVoidArray* listeners = GetListenersByType(aType, aKey, PR_TRUE);

  //We asked the GetListenersByType to create the array if it had to.  If it didn't
  //then we're out of memory (or a bug was added which passed in an unsupported
  //event type)
  if (!listeners) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // For mutation listeners, we need to update the global bit on the DOM window.
  // Otherwise we won't actually fire the mutation event.
  if (aType == eEventArrayType_Mutation) {
    // Go from our target to the nearest enclosing DOM window.
    nsCOMPtr<nsIScriptGlobalObject> global;
    nsCOMPtr<nsIDocument> document;
    nsCOMPtr<nsIContent> content(do_QueryInterface(mTarget));
    if (content)
      document = content->GetOwnerDoc();
    else document = do_QueryInterface(mTarget);
    if (document)
      global = document->GetScriptGlobalObject();
    else global = do_QueryInterface(mTarget);
    if (global) {
      nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(global));
      window->SetMutationListeners(aSubType);
    }
  }
  
  PRBool isSame = PR_FALSE;
  PRUint16 group = 0;
  nsCOMPtr<nsIDOMEventGroup> sysGroup;
  GetSystemEventGroupLM(getter_AddRefs(sysGroup));
  if (sysGroup) {
    sysGroup->IsSameEventGroup(aEvtGrp, &isSame);
    if (isSame) {
      group = NS_EVENT_FLAG_SYSTEM_EVENT;
    }
  }

  PRBool found = PR_FALSE;
  nsListenerStruct* ls;

  for (PRInt32 i=0; i<listeners->Count(); i++) {
    ls = (nsListenerStruct*)listeners->ElementAt(i);
    if (ls->mListener == aListener && ls->mFlags == aFlags &&
        ls->mGroupFlags == group) {
      ls->mSubType |= aSubType;
      found = PR_TRUE;
      break;
    }
  }

  if (!found) {
    ls = PR_NEW(nsListenerStruct);
    if (ls) {
      ls->mListener = aListener;
      ls->mFlags = aFlags;
      ls->mSubType = aSubType;
      ls->mSubTypeCapture = NS_EVENT_BITS_NONE;
      ls->mHandlerIsString = 0;
      ls->mGroupFlags = group;
      listeners->AppendElement((void*)ls);
      NS_ADDREF(aListener);
    }
  }

  return NS_OK;
}

nsresult
nsEventListenerManager::RemoveEventListener(nsIDOMEventListener *aListener, 
                                            EventArrayType aType, 
                                            PRInt32 aSubType,
                                            nsHashKey* aKey,
                                            PRInt32 aFlags,
                                            nsIDOMEventGroup* aEvtGrp)
{
  nsVoidArray* listeners = GetListenersByType(aType, aKey, PR_FALSE);

  if (!listeners) {
    return NS_OK;
  }

  nsListenerStruct* ls;
  aFlags &= ~NS_PRIV_EVENT_UNTRUSTED_PERMITTED;

  for (PRInt32 i=0; i<listeners->Count(); i++) {
    ls = (nsListenerStruct*)listeners->ElementAt(i);
    if (ls->mListener == aListener &&
        (ls->mFlags & ~NS_PRIV_EVENT_UNTRUSTED_PERMITTED) == aFlags) {
      ls->mSubType &= ~aSubType;
      if (ls->mSubType == NS_EVENT_BITS_NONE) {
        NS_RELEASE(ls->mListener);
        listeners->RemoveElement((void*)ls);
        PR_DELETE(ls);
      }
      break;
    }
  }

  return NS_OK;
}

nsresult
nsEventListenerManager::AddEventListenerByIID(nsIDOMEventListener *aListener, 
                                              const nsIID& aIID,
                                              PRInt32 aFlags)
{
  AddEventListener(aListener, GetTypeForIID(aIID), NS_EVENT_BITS_NONE, nsnull,
                   aFlags, nsnull);
  return NS_OK;
}

NS_IMETHODIMP
nsEventListenerManager::RemoveEventListenerByIID(nsIDOMEventListener *aListener, 
                                                 const nsIID& aIID,
                                                 PRInt32 aFlags)
{
  RemoveEventListener(aListener, GetTypeForIID(aIID), NS_EVENT_BITS_NONE,
                      nsnull, aFlags, nsnull);
  return NS_OK;
}

nsresult
nsEventListenerManager::GetIdentifiersForType(nsIAtom* aType,
                                              EventArrayType* aArrayType,
                                              PRInt32* aFlags)
{
  if (aType == nsLayoutAtoms::onmousedown) {
    *aArrayType = eEventArrayType_Mouse;
    *aFlags = NS_EVENT_BITS_MOUSE_MOUSEDOWN;
  }
  else if (aType == nsLayoutAtoms::onmouseup) {
    *aArrayType = eEventArrayType_Mouse;
    *aFlags = NS_EVENT_BITS_MOUSE_MOUSEUP;
  }
  else if (aType == nsLayoutAtoms::onclick) {
    *aArrayType = eEventArrayType_Mouse;
    *aFlags = NS_EVENT_BITS_MOUSE_CLICK;
  }
  else if (aType == nsLayoutAtoms::ondblclick) {
    *aArrayType = eEventArrayType_Mouse;
    *aFlags = NS_EVENT_BITS_MOUSE_DBLCLICK;
  }
  else if (aType == nsLayoutAtoms::onmouseover) {
    *aArrayType = eEventArrayType_Mouse;
    *aFlags = NS_EVENT_BITS_MOUSE_MOUSEOVER;
  }
  else if (aType == nsLayoutAtoms::onmouseout) {
    *aArrayType = eEventArrayType_Mouse;
    *aFlags = NS_EVENT_BITS_MOUSE_MOUSEOUT;
  }
  else if (aType == nsLayoutAtoms::onkeydown) {
    *aArrayType = eEventArrayType_Key;
    *aFlags = NS_EVENT_BITS_KEY_KEYDOWN;
  }
  else if (aType == nsLayoutAtoms::onkeyup) {
    *aArrayType = eEventArrayType_Key;
    *aFlags = NS_EVENT_BITS_KEY_KEYUP;
  }
  else if (aType == nsLayoutAtoms::onkeypress) {
    *aArrayType = eEventArrayType_Key;
    *aFlags = NS_EVENT_BITS_KEY_KEYPRESS;
  }
  else if (aType == nsLayoutAtoms::onmousemove) {
    *aArrayType = eEventArrayType_MouseMotion;
    *aFlags = NS_EVENT_BITS_MOUSEMOTION_MOUSEMOVE;
  }
  else if (aType == nsLayoutAtoms::oncontextmenu) {
    *aArrayType = eEventArrayType_ContextMenu;
    *aFlags = NS_EVENT_BITS_CONTEXTMENU;
  }
  else if (aType == nsLayoutAtoms::onfocus) {
    *aArrayType = eEventArrayType_Focus;
    *aFlags = NS_EVENT_BITS_FOCUS_FOCUS;
  }
  else if (aType == nsLayoutAtoms::onblur) {
    *aArrayType = eEventArrayType_Focus;
    *aFlags = NS_EVENT_BITS_FOCUS_BLUR;
  }
  else if (aType == nsLayoutAtoms::onsubmit) {
    *aArrayType = eEventArrayType_Form;
    *aFlags = NS_EVENT_BITS_FORM_SUBMIT;
  }
  else if (aType == nsLayoutAtoms::onreset) {
    *aArrayType = eEventArrayType_Form;
    *aFlags = NS_EVENT_BITS_FORM_RESET;
  }
  else if (aType == nsLayoutAtoms::onchange) {
    *aArrayType = eEventArrayType_Form;
    *aFlags = NS_EVENT_BITS_FORM_CHANGE;
  }
  else if (aType == nsLayoutAtoms::onselect) {
    *aArrayType = eEventArrayType_Form;
    *aFlags = NS_EVENT_BITS_FORM_SELECT;
  }
  else if (aType == nsLayoutAtoms::oninput) {
    *aArrayType = eEventArrayType_Form;
    *aFlags = NS_EVENT_BITS_FORM_INPUT;
  }
  else if (aType == nsLayoutAtoms::onload) {
    *aArrayType = eEventArrayType_Load;
    *aFlags = NS_EVENT_BITS_LOAD_LOAD;
  }
  else if (aType == nsLayoutAtoms::onbeforeunload) {
    *aArrayType = eEventArrayType_Load;
    *aFlags = NS_EVENT_BITS_LOAD_BEFORE_UNLOAD;
  }
  else if (aType == nsLayoutAtoms::onunload) {
    *aArrayType = eEventArrayType_Load;
    *aFlags = NS_EVENT_BITS_LOAD_UNLOAD;
  }
  else if (aType == nsLayoutAtoms::onabort) {
    *aArrayType = eEventArrayType_Load;
    *aFlags = NS_EVENT_BITS_LOAD_ABORT;
  }
  else if (aType == nsLayoutAtoms::onerror) {
    *aArrayType = eEventArrayType_Load;
    *aFlags = NS_EVENT_BITS_LOAD_ERROR;
  }
  else if (aType == nsLayoutAtoms::onpaint) {
    *aArrayType = eEventArrayType_Paint;
    *aFlags = NS_EVENT_BITS_PAINT_PAINT;
  }
  else if (aType == nsLayoutAtoms::onresize) {
    *aArrayType = eEventArrayType_Paint;
    *aFlags = NS_EVENT_BITS_PAINT_RESIZE;
  }
  else if (aType == nsLayoutAtoms::onscroll) {
    *aArrayType = eEventArrayType_Paint;
    *aFlags = NS_EVENT_BITS_PAINT_SCROLL;
  } // extened this to handle IME related events
  else if (aType == nsLayoutAtoms::onpopupshowing) {
    *aArrayType = eEventArrayType_XUL; 
    *aFlags = NS_EVENT_BITS_XUL_POPUP_SHOWING;
  }
  else if (aType == nsLayoutAtoms::onpopupshown) {
    *aArrayType = eEventArrayType_XUL; 
    *aFlags = NS_EVENT_BITS_XUL_POPUP_SHOWN;
  }
  else if (aType == nsLayoutAtoms::onpopuphiding) {
    *aArrayType = eEventArrayType_XUL; 
    *aFlags = NS_EVENT_BITS_XUL_POPUP_HIDING;
  }
  else if (aType == nsLayoutAtoms::onpopuphidden) {
    *aArrayType = eEventArrayType_XUL; 
    *aFlags = NS_EVENT_BITS_XUL_POPUP_HIDDEN;
  }
  else if (aType == nsLayoutAtoms::onclose) {
    *aArrayType = eEventArrayType_XUL; 
    *aFlags = NS_EVENT_BITS_XUL_CLOSE;
  }
  else if (aType == nsLayoutAtoms::oncommand) {
    *aArrayType = eEventArrayType_XUL; 
    *aFlags = NS_EVENT_BITS_XUL_COMMAND;
  }
  else if (aType == nsLayoutAtoms::onbroadcast) {
    *aArrayType = eEventArrayType_XUL;
    *aFlags = NS_EVENT_BITS_XUL_BROADCAST;
  }
  else if (aType == nsLayoutAtoms::oncommandupdate) {
    *aArrayType = eEventArrayType_XUL;
    *aFlags = NS_EVENT_BITS_XUL_COMMAND_UPDATE;
  }
  else if (aType == nsLayoutAtoms::onoverflow) {
    *aArrayType = eEventArrayType_Scroll;
    *aFlags = NS_EVENT_BITS_SCROLLPORT_OVERFLOW;
  }
  else if (aType == nsLayoutAtoms::onunderflow) {
    *aArrayType = eEventArrayType_Scroll;
    *aFlags = NS_EVENT_BITS_SCROLLPORT_UNDERFLOW;
  }
  else if (aType == nsLayoutAtoms::onoverflowchanged) {
    *aArrayType = eEventArrayType_Scroll;
    *aFlags = NS_EVENT_BITS_SCROLLPORT_OVERFLOWCHANGED;
  }
  else if (aType == nsLayoutAtoms::ondragenter) {
    *aArrayType = eEventArrayType_Drag;
    *aFlags = NS_EVENT_BITS_DRAG_ENTER;
  }
  else if (aType == nsLayoutAtoms::ondragover) {
    *aArrayType = eEventArrayType_Drag; 
    *aFlags = NS_EVENT_BITS_DRAG_OVER;
  }
  else if (aType == nsLayoutAtoms::ondragexit) {
    *aArrayType = eEventArrayType_Drag; 
    *aFlags = NS_EVENT_BITS_DRAG_EXIT;
  }
  else if (aType == nsLayoutAtoms::ondragdrop) {
    *aArrayType = eEventArrayType_Drag; 
    *aFlags = NS_EVENT_BITS_DRAG_DROP;
  }
  else if (aType == nsLayoutAtoms::ondraggesture) {
    *aArrayType = eEventArrayType_Drag; 
    *aFlags = NS_EVENT_BITS_DRAG_GESTURE;
  }
  else if (aType == nsLayoutAtoms::onDOMSubtreeModified) {
    *aArrayType = eEventArrayType_Mutation;
    *aFlags = NS_EVENT_BITS_MUTATION_SUBTREEMODIFIED;
  }
  else if (aType == nsLayoutAtoms::onDOMNodeInserted) {
    *aArrayType = eEventArrayType_Mutation;
    *aFlags = NS_EVENT_BITS_MUTATION_NODEINSERTED;
  }
  else if (aType == nsLayoutAtoms::onDOMNodeRemoved) {
    *aArrayType = eEventArrayType_Mutation;
    *aFlags = NS_EVENT_BITS_MUTATION_NODEREMOVED;
  }
  else if (aType == nsLayoutAtoms::onDOMNodeInsertedIntoDocument) {
    *aArrayType = eEventArrayType_Mutation;
    *aFlags = NS_EVENT_BITS_MUTATION_NODEINSERTEDINTODOCUMENT;
  }
  else if (aType == nsLayoutAtoms::onDOMNodeRemovedFromDocument) {
    *aArrayType = eEventArrayType_Mutation;
    *aFlags = NS_EVENT_BITS_MUTATION_NODEREMOVEDFROMDOCUMENT;
  }
  else if (aType == nsLayoutAtoms::onDOMAttrModified) {
    *aArrayType = eEventArrayType_Mutation;
    *aFlags = NS_EVENT_BITS_MUTATION_ATTRMODIFIED;
  }
  else if (aType == nsLayoutAtoms::onDOMCharacterDataModified) {
    *aArrayType = eEventArrayType_Mutation;
    *aFlags = NS_EVENT_BITS_MUTATION_CHARACTERDATAMODIFIED;
  }
  else if (aType == nsLayoutAtoms::onDOMActivate) {
    *aArrayType = eEventArrayType_DOMUI;
    *aFlags = NS_EVENT_BITS_UI_ACTIVATE;
  }
  else if (aType == nsLayoutAtoms::onDOMFocusIn) {
    *aArrayType = eEventArrayType_DOMUI;
    *aFlags = NS_EVENT_BITS_UI_FOCUSIN;
  }
  else if (aType == nsLayoutAtoms::onDOMFocusOut) {
    *aArrayType = eEventArrayType_DOMUI;
    *aFlags = NS_EVENT_BITS_UI_FOCUSOUT;
  }
  else if (aType == nsLayoutAtoms::oncompositionstart) {
    *aArrayType = eEventArrayType_Composition;
    *aFlags = NS_EVENT_BITS_COMPOSITION_START;
  }
  else if (aType == nsLayoutAtoms::oncompositionend) {
    *aArrayType = eEventArrayType_Composition;
    *aFlags = NS_EVENT_BITS_COMPOSITION_END;
  }
  else if (aType == nsLayoutAtoms::ontext) {
    *aArrayType = eEventArrayType_Text;
    *aFlags = NS_EVENT_BITS_TEXT_TEXT;
  }
  else if (aType == nsLayoutAtoms::onpageshow) {
    *aArrayType = eEventArrayType_PageTransition;
    *aFlags = NS_EVENT_BITS_PAGETRANSITION_SHOW;
  }
  else if (aType == nsLayoutAtoms::onpagehide) {
    *aArrayType = eEventArrayType_PageTransition;
    *aFlags = NS_EVENT_BITS_PAGETRANSITION_HIDE;
  }
#ifdef MOZ_SVG
  else if (aType == nsLayoutAtoms::onSVGLoad) {
    *aArrayType = eEventArrayType_SVG;
    *aFlags = NS_EVENT_BITS_SVG_LOAD;
  }
  else if (aType == nsLayoutAtoms::onSVGUnload) {
    *aArrayType = eEventArrayType_SVG;
    *aFlags = NS_EVENT_BITS_SVG_UNLOAD;
  }
  else if (aType == nsLayoutAtoms::onSVGAbort) {
    *aArrayType = eEventArrayType_SVG;
    *aFlags = NS_EVENT_BITS_SVG_ABORT;
  }
  else if (aType == nsLayoutAtoms::onSVGError) {
    *aArrayType = eEventArrayType_SVG;
    *aFlags = NS_EVENT_BITS_SVG_ERROR;
  }
  else if (aType == nsLayoutAtoms::onSVGResize) {
    *aArrayType = eEventArrayType_SVG;
    *aFlags = NS_EVENT_BITS_SVG_RESIZE;
  }
  else if (aType == nsLayoutAtoms::onSVGScroll) {
    *aArrayType = eEventArrayType_SVG;
    *aFlags = NS_EVENT_BITS_SVG_SCROLL;
  }
  else if (aType == nsLayoutAtoms::onSVGZoom) {
    *aArrayType = eEventArrayType_SVGZoom;
    *aFlags = NS_EVENT_BITS_SVGZOOM_ZOOM;
  }
#endif // MOZ_SVG
  else {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsEventListenerManager::AddEventListenerByType(nsIDOMEventListener *aListener, 
                                               const nsAString& aType,
                                               PRInt32 aFlags,
                                               nsIDOMEventGroup* aEvtGrp)
{
  PRInt32 subType;
  EventArrayType arrayType;
  nsCOMPtr<nsIAtom> atom = do_GetAtom(NS_LITERAL_STRING("on") + aType);

  if (NS_OK == GetIdentifiersForType(atom, &arrayType, &subType)) {
    AddEventListener(aListener, arrayType, subType, nsnull, aFlags, aEvtGrp);
  }
  else {
    const nsPromiseFlatString& flatString = PromiseFlatString(aType); 
    nsStringKey key(flatString);
    AddEventListener(aListener, eEventArrayType_Hash, NS_EVENT_BITS_NONE, &key, aFlags, aEvtGrp);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEventListenerManager::RemoveEventListenerByType(nsIDOMEventListener *aListener, 
                                                  const nsAString& aType,
                                                  PRInt32 aFlags,
                                                  nsIDOMEventGroup* aEvtGrp)
{
  PRInt32 subType;
  EventArrayType arrayType;
  nsCOMPtr<nsIAtom> atom = do_GetAtom(NS_LITERAL_STRING("on") + aType);

  if (NS_OK == GetIdentifiersForType(atom, &arrayType, &subType)) {
    RemoveEventListener(aListener, arrayType, subType, nsnull, aFlags, aEvtGrp);
  }
  else {
    const nsPromiseFlatString& flatString = PromiseFlatString(aType); 
    nsStringKey key(flatString);
    RemoveEventListener(aListener, eEventArrayType_Hash, NS_EVENT_BITS_NONE, &key, aFlags, aEvtGrp);
  }

  return NS_OK;
}

nsListenerStruct*
nsEventListenerManager::FindJSEventListener(EventArrayType aType)
{
  nsVoidArray *listeners = GetListenersByType(aType, nsnull, PR_FALSE);
  if (listeners) {
    // Run through the listeners for this type and see if a script
    // listener is registered
    nsListenerStruct *ls;
    for (PRInt32 i=0; i < listeners->Count(); i++) {
      ls = (nsListenerStruct*)listeners->ElementAt(i);
      if (ls->mFlags & NS_PRIV_EVENT_FLAG_SCRIPT) {
        return ls;
      }
    }
  }

  return nsnull;
}

nsresult
nsEventListenerManager::SetJSEventListener(nsIScriptContext *aContext,
                                           JSObject *aScopeObject,
                                           nsISupports *aObject,
                                           nsIAtom* aName,
                                           PRBool aIsString,
                                           PRBool aPermitUntrustedEvents)
{
  nsresult rv = NS_OK;
  nsListenerStruct *ls;
  PRInt32 flags;
  EventArrayType arrayType;

  NS_ENSURE_SUCCESS(GetIdentifiersForType(aName, &arrayType, &flags),
                    NS_ERROR_FAILURE);

  ls = FindJSEventListener(arrayType);

  if (nsnull == ls) {
    // If we didn't find a script listener or no listeners existed
    // create and add a new one.
    nsCOMPtr<nsIDOMEventListener> scriptListener;
    rv = NS_NewJSEventListener(aContext, aScopeObject, aObject,
                               getter_AddRefs(scriptListener));
    if (NS_SUCCEEDED(rv)) {
      AddEventListener(scriptListener, arrayType, NS_EVENT_BITS_NONE, nsnull,
                       NS_EVENT_FLAG_BUBBLE | NS_PRIV_EVENT_FLAG_SCRIPT, nsnull);

      ls = FindJSEventListener(arrayType);
    }
  }

  if (NS_SUCCEEDED(rv) && ls) {
    // Set flag to indicate possible need for compilation later
    if (aIsString) {
      ls->mHandlerIsString |= flags;
    }
    else {
      ls->mHandlerIsString &= ~flags;
    }

    // Set subtype flags based on event
    ls->mSubType |= flags;

    if (aPermitUntrustedEvents) {
      ls->mFlags |= NS_PRIV_EVENT_UNTRUSTED_PERMITTED;
    }
  }

  return rv;
}

NS_IMETHODIMP
nsEventListenerManager::AddScriptEventListener(nsISupports *aObject,
                                               nsIAtom *aName,
                                               const nsAString& aBody,
                                               PRBool aDeferCompilation,
                                               PRBool aPermitUntrustedEvents)
{
  nsIScriptContext *context = nsnull;
  JSContext* cx = nsnull;

  nsCOMPtr<nsIContent> content(do_QueryInterface(aObject));

  nsCOMPtr<nsIDocument> doc;

  nsISupports *objiSupp = aObject;

  JSObject *scope = nsnull;

  if (content) {
    // Try to get context from doc
    doc = content->GetOwnerDoc();
    nsIScriptGlobalObject *global;

    if (doc && (global = doc->GetScriptGlobalObject())) {
      context = global->GetContext();
      scope = global->GetGlobalJSObject();
    }
  } else {
    nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(aObject));
    nsCOMPtr<nsIScriptGlobalObject> global;
    if (win) {
      NS_ASSERTION(win->IsInnerWindow(),
                   "Event listener added to outer window!");

      nsCOMPtr<nsIDOMDocument> domdoc;
      win->GetDocument(getter_AddRefs(domdoc));
      doc = do_QueryInterface(domdoc);
      global = do_QueryInterface(win);
    } else {
      doc = do_QueryInterface(aObject);

      if (doc) {
        global = doc->GetScriptGlobalObject();
      } else {
        global = do_QueryInterface(aObject);
      }
    }
    if (global) {
      context = global->GetContext();
      scope = global->GetGlobalJSObject();
    }
  }

  if (!context) {
    // Get JSContext from stack, or use the safe context (and hidden
    // window global) if no JS is running.
    nsCOMPtr<nsIThreadJSContextStack> stack =
      do_GetService("@mozilla.org/js/xpc/ContextStack;1");
    NS_ENSURE_TRUE(stack, NS_ERROR_FAILURE);
    NS_ENSURE_SUCCESS(stack->Peek(&cx), NS_ERROR_FAILURE);

    if (!cx) {
      stack->GetSafeJSContext(&cx);
      NS_ENSURE_TRUE(cx, NS_ERROR_FAILURE);
    }

    context = nsJSUtils::GetDynamicScriptContext(cx);
    NS_ENSURE_TRUE(context, NS_ERROR_FAILURE);

    scope = ::JS_GetGlobalObject(cx);
  } else if (!scope) {
    NS_ERROR("Context reachable, but no scope reachable in "
             "AddScriptEventListener()!");

    return NS_ERROR_NOT_AVAILABLE;
  }

  nsresult rv;

  if (!aDeferCompilation) {
    JSContext *cx = (JSContext *)context->GetNativeContext();

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    rv = nsContentUtils::XPConnect()->WrapNative(cx, scope, aObject,
                                                 NS_GET_IID(nsISupports),
                                                 getter_AddRefs(holder));
    NS_ENSURE_SUCCESS(rv, rv);

    // Since JSEventListeners only have a raw nsISupports pointer, it's
    // important that it point to the same object that the WrappedNative wraps.
    // (In the case of a tearoff, the tearoff will not persist).
    nsCOMPtr<nsIXPConnectWrappedNative> wrapper = do_QueryInterface(holder);
    NS_ASSERTION(wrapper, "wrapper must impl nsIXPConnectWrappedNative");

    objiSupp = wrapper->Native();

    JSObject *scriptObject = nsnull;

    rv = holder->GetJSObject(&scriptObject);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIScriptEventHandlerOwner> handlerOwner =
      do_QueryInterface(aObject);

    void *handler = nsnull;
    PRBool done = PR_FALSE;

    if (handlerOwner) {
      rv = handlerOwner->GetCompiledEventHandler(aName, &handler);
      if (NS_SUCCEEDED(rv) && handler) {
        rv = context->BindCompiledEventHandler(scriptObject, aName, handler);
        if (NS_FAILED(rv))
          return rv;
        done = PR_TRUE;
      }
    }

    if (!done) {
      PRUint32 lineNo = 0;
      nsCAutoString url (NS_LITERAL_CSTRING("-moz-evil:lying-event-listener"));
      if (doc) {
        nsIURI *uri = doc->GetDocumentURI();
        if (uri) {
          uri->GetSpec(url);
          lineNo = 1;
        }
      }

      if (handlerOwner) {
        // Always let the handler owner compile the event handler, as
        // it may want to use a special context or scope object.
        rv = handlerOwner->CompileEventHandler(context, scriptObject, aName,
                                               aBody, url.get(), lineNo, &handler);
      }
      else {
        PRInt32 nameSpace = kNameSpaceID_Unknown;
        if (content)
          nameSpace = content->GetNameSpaceID();
        else if (doc) {
          nsCOMPtr<nsIContent> root = doc->GetRootContent();
          if (root)
            nameSpace = root->GetNameSpaceID();
        }
        const char *eventName = nsContentUtils::GetEventArgName(nameSpace);

        rv = context->CompileEventHandler(scriptObject, aName, eventName,
                                          aBody,
                                          url.get(), lineNo,
                                          (handlerOwner != nsnull),
                                          &handler);
      }
      if (NS_FAILED(rv)) return rv;
    }
  }

  return SetJSEventListener(context, scope, objiSupp, aName, aDeferCompilation,
                            aPermitUntrustedEvents);
}

nsresult
nsEventListenerManager::RemoveScriptEventListener(nsIAtom *aName)
{
  nsresult result = NS_OK;
  nsListenerStruct *ls;
  PRInt32 flags;
  EventArrayType arrayType;

  NS_ENSURE_SUCCESS(GetIdentifiersForType(aName, &arrayType, &flags),
                    NS_ERROR_FAILURE);
  ls = FindJSEventListener(arrayType);

  if (ls) {
    ls->mSubType &= ~flags;
    if (ls->mSubType == NS_EVENT_BITS_NONE) {
      NS_RELEASE(ls->mListener);

      //Get the listeners array so we can remove ourselves from it
      nsVoidArray* listeners;
      listeners = GetListenersByType(arrayType, nsnull, PR_FALSE);
      NS_ENSURE_TRUE(listeners, NS_ERROR_FAILURE);
      listeners->RemoveElement((void*)ls);
      PR_DELETE(ls);
    }
  }

  return result;
}

jsval
nsEventListenerManager::sAddListenerID = JSVAL_VOID;

NS_IMETHODIMP
nsEventListenerManager::RegisterScriptEventListener(nsIScriptContext *aContext,
                                                    JSObject *aScopeObject,
                                                    nsISupports *aObject, 
                                                    nsIAtom *aName)
{
  // Check that we have access to set an event listener. Prevents
  // snooping attacks across domains by setting onkeypress handlers,
  // for instance.
  // You'd think it'd work just to get the JSContext from aContext,
  // but that's actually the JSContext whose private object parents
  // the object in aObject.
  nsresult rv;
  nsCOMPtr<nsIJSContextStack> stack =
    do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);
  if (NS_FAILED(rv))
    return rv;
  JSContext *cx;
  if (NS_FAILED(rv = stack->Peek(&cx)))
    return rv;

  JSContext *current_cx = (JSContext *)aContext->GetNativeContext();

  nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
  rv = nsContentUtils::XPConnect()->
    WrapNative(current_cx, aScopeObject, aObject, NS_GET_IID(nsISupports),
               getter_AddRefs(holder));
  NS_ENSURE_SUCCESS(rv, rv);

  // Since JSEventListeners only have a raw nsISupports pointer, it's
  // important that it point to the same object that the WrappedNative wraps.
  // (In the case of a tearoff, the tearoff will not persist).
  nsCOMPtr<nsIXPConnectWrappedNative> wrapper = do_QueryInterface(holder);
  NS_ASSERTION(wrapper, "wrapper must impl nsIXPConnectWrappedNative");

  JSObject *jsobj = nsnull;

  rv = holder->GetJSObject(&jsobj);
  NS_ENSURE_SUCCESS(rv, rv);

  if (cx) {
    if (sAddListenerID == JSVAL_VOID) {
      sAddListenerID =
        STRING_TO_JSVAL(::JS_InternString(cx, "addEventListener"));
    }

    rv = nsContentUtils::GetSecurityManager()->
      CheckPropertyAccess(cx, jsobj,
                          "EventTarget",
                          sAddListenerID,
                          nsIXPCSecurityManager::ACCESS_SET_PROPERTY);
    if (NS_FAILED(rv)) {
      // XXX set pending exception on the native call context?
      return rv;
    }
  }

  // Untrusted events are always permitted for non-chrome script
  // handlers.
  return SetJSEventListener(aContext, aScopeObject, wrapper->Native(), aName,
                            PR_FALSE, !nsContentUtils::IsCallerChrome());
}

nsresult
nsEventListenerManager::CompileScriptEventListener(nsIScriptContext *aContext, 
                                                   JSObject *aScopeObject,
                                                   nsISupports *aObject, 
                                                   nsIAtom *aName,
                                                   PRBool *aDidCompile)
{
  nsresult rv = NS_OK;
  nsListenerStruct *ls;
  PRInt32 subType;
  EventArrayType arrayType;

  *aDidCompile = PR_FALSE;

  rv = GetIdentifiersForType(aName, &arrayType, &subType);
  NS_ENSURE_SUCCESS(rv, rv);

  ls = FindJSEventListener(arrayType);

  if (!ls) {
    //nothing to compile
    return NS_OK;
  }

  if (ls->mHandlerIsString & subType) {
    rv = CompileEventHandlerInternal(aContext, aScopeObject, aObject, aName,
                                     ls, /*XXX fixme*/nsnull, subType);
  }

  // Set *aDidCompile to true even if we didn't really compile
  // anything right now, if we get here it means that this event
  // handler has been compiled at some point, that's good enough for
  // us.

  *aDidCompile = PR_TRUE;

  return rv;
}

nsresult
nsEventListenerManager::CompileEventHandlerInternal(nsIScriptContext *aContext,
                                                    JSObject *aScopeObject,
                                                    nsISupports *aObject,
                                                    nsIAtom *aName,
                                                    nsListenerStruct *aListenerStruct,
                                                    nsIDOMEventTarget* aCurrentTarget,
                                                    PRUint32 aSubType)
{
  nsresult result = NS_OK;

  JSContext *cx = (JSContext *)aContext->GetNativeContext();

  nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
  result = nsContentUtils::XPConnect()->WrapNative(cx, aScopeObject, aObject,
                                                   NS_GET_IID(nsISupports),
                                                   getter_AddRefs(holder));
  NS_ENSURE_SUCCESS(result, result);

  JSObject *jsobj = nsnull;

  result = holder->GetJSObject(&jsobj);
  NS_ENSURE_SUCCESS(result, result);

  nsCOMPtr<nsIScriptEventHandlerOwner> handlerOwner =
    do_QueryInterface(aObject);
  void* handler = nsnull;

  if (handlerOwner) {
    result = handlerOwner->GetCompiledEventHandler(aName, &handler);
    if (NS_SUCCEEDED(result) && handler) {
      result = aContext->BindCompiledEventHandler(jsobj, aName, handler);
      aListenerStruct->mHandlerIsString &= ~aSubType;
    }
  }

  if (aListenerStruct->mHandlerIsString & aSubType) {
    // This should never happen for anything but content
    // XXX I don't like that we have to reference content
    // from here. The alternative is to store the event handler
    // string on the JS object itself.
    nsCOMPtr<nsIContent> content = do_QueryInterface(aObject);
    NS_ASSERTION(content, "only content should have event handler attributes");
    if (content) {
      nsAutoString handlerBody;
      nsIAtom* attrName = aName;
#ifdef MOZ_SVG
      if (aName == nsLayoutAtoms::onSVGLoad)
        attrName = nsSVGAtoms::onload;
      else if (aName == nsLayoutAtoms::onSVGUnload)
        attrName = nsSVGAtoms::onunload;
      else if (aName == nsLayoutAtoms::onSVGAbort)
        attrName = nsSVGAtoms::onabort;
      else if (aName == nsLayoutAtoms::onSVGError)
        attrName = nsSVGAtoms::onerror;
      else if (aName == nsLayoutAtoms::onSVGResize)
        attrName = nsSVGAtoms::onresize;
      else if (aName == nsLayoutAtoms::onSVGScroll)
        attrName = nsSVGAtoms::onscroll;
      else if (aName == nsLayoutAtoms::onSVGZoom)
        attrName = nsSVGAtoms::onzoom;
#endif // MOZ_SVG

      result = content->GetAttr(kNameSpaceID_None, attrName, handlerBody);

      if (NS_SUCCEEDED(result)) {
        PRUint32 lineNo = 0;
        nsCAutoString url (NS_LITERAL_CSTRING("javascript:alert('TODO: FIXME')"));
        nsCOMPtr<nsIDocument> doc = do_QueryInterface(aCurrentTarget);
        if (!doc) {
          nsCOMPtr<nsIContent> content = do_QueryInterface(aCurrentTarget);
          if (content)
            doc = content->GetOwnerDoc();
        }
        if (doc) {
          nsIURI *uri = doc->GetDocumentURI();
          if (uri) {
            uri->GetSpec(url);
            lineNo = 1;
          }
        }

        if (handlerOwner) {
          // Always let the handler owner compile the event
          // handler, as it may want to use a special
          // context or scope object.
          result = handlerOwner->CompileEventHandler(aContext, jsobj, aName,
                                                     handlerBody,
                                                     url.get(), lineNo,
                                                     &handler);
        }
        else {
          const char *eventName =
            nsContentUtils::GetEventArgName(content->GetNameSpaceID());

          result = aContext->CompileEventHandler(jsobj, aName, eventName,
                                                 handlerBody,
                                                 url.get(), lineNo,
                                                 (handlerOwner != nsnull),
                                                 &handler);
        }

        if (NS_SUCCEEDED(result)) {
          aListenerStruct->mHandlerIsString &= ~aSubType;
        }
      }
    }
  }

  return result;
}

nsresult
nsEventListenerManager::HandleEventSubType(nsListenerStruct* aListenerStruct,
                                           nsIDOMEvent* aDOMEvent,
                                           nsIDOMEventTarget* aCurrentTarget,
                                           PRUint32 aSubType,
                                           PRUint32 aPhaseFlags)
{
  nsresult result = NS_OK;

  // If this is a script handler and we haven't yet
  // compiled the event handler itself
  if (aListenerStruct->mFlags & NS_PRIV_EVENT_FLAG_SCRIPT) {
    // If we're not in the capture phase we must *NOT* have capture flags
    // set.  Compiled script handlers are one or the other, not both.
    if (aPhaseFlags & NS_EVENT_FLAG_BUBBLE && !aPhaseFlags & NS_EVENT_FLAG_INIT) {
      if (aListenerStruct->mSubTypeCapture & aSubType) {
        return result;
      }
    }
    // If we're in the capture phase we must have capture flags set.
    else if (aPhaseFlags & NS_EVENT_FLAG_CAPTURE && !aPhaseFlags & NS_EVENT_FLAG_INIT) {
      if (!(aListenerStruct->mSubTypeCapture & aSubType)) {
        return result;
      }
    }
    if (aListenerStruct->mHandlerIsString & aSubType) {

      nsCOMPtr<nsIJSEventListener> jslistener = do_QueryInterface(aListenerStruct->mListener);
      if (jslistener) {
        nsAutoString eventString;
        if (NS_SUCCEEDED(aDOMEvent->GetType(eventString))) {
          nsCOMPtr<nsIAtom> atom = do_GetAtom(NS_LITERAL_STRING("on") + eventString);

          result = CompileEventHandlerInternal(jslistener->GetEventContext(),
                                               jslistener->GetEventScope(),
                                               jslistener->GetEventTarget(),
                                               atom, aListenerStruct,
                                               aCurrentTarget,
                                               aSubType);
        }
      }
    }
  }

  // nsCxPusher will push and pop (automatically) the current cx onto the
  // context stack
  nsCxPusher pusher;

  if (NS_SUCCEEDED(result) && pusher.Push(aCurrentTarget)) {
    nsCOMPtr<nsIPrivateDOMEvent> aPrivDOMEvent(do_QueryInterface(aDOMEvent));
    aPrivDOMEvent->SetCurrentTarget(aCurrentTarget);
    // Hold a strong ref to the event listener so it won't die while
    // handling the event.
    nsCOMPtr<nsIDOMEventListener> listener = aListenerStruct->mListener;
    result = listener->HandleEvent(aDOMEvent);
    aPrivDOMEvent->SetCurrentTarget(nsnull);
  }

  return result;
}

/**
* Causes a check for event listeners and processing by them if they exist.
* @param an event listener
*/

nsresult
nsEventListenerManager::HandleEvent(nsPresContext* aPresContext,
                                    nsEvent* aEvent, nsIDOMEvent** aDOMEvent,
                                    nsIDOMEventTarget* aCurrentTarget,
                                    PRUint32 aFlags,
                                    nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  nsresult ret = NS_OK;

  if (aEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH) {
    return ret;
  }

  if (aFlags & NS_EVENT_FLAG_INIT) {
    aFlags |= (NS_EVENT_FLAG_BUBBLE | NS_EVENT_FLAG_CAPTURE);
  }
  //Set the value of the internal PreventDefault flag properly based on aEventStatus
  if (*aEventStatus == nsEventStatus_eConsumeNoDefault) {
    aEvent->flags |= NS_EVENT_FLAG_NO_DEFAULT;
  }
  PRUint16 currentGroup = aFlags & NS_EVENT_FLAG_SYSTEM_EVENT;

  /* Without this addref, certain events, notably ones bound to
     keys which cause window deletion, can destroy this object
     before we're ready. */
  nsCOMPtr<nsIEventListenerManager> kungFuDeathGrip(this);
  nsVoidArray *listeners = nsnull;

  if (aEvent->message == NS_CONTEXTMENU || aEvent->message == NS_CONTEXTMENU_KEY) {
    ret = FixContextMenuEvent(aPresContext, aCurrentTarget, aEvent, aDOMEvent);
    if (NS_FAILED(ret)) {
      NS_WARNING("failed to fix context menu event target");
      ret = NS_OK;
    }
  }


  const EventTypeData* typeData = nsnull;
  const EventDispatchData* dispData = nsnull;

  if (aEvent->message == NS_USER_DEFINED_EVENT) {
    listeners = GetListenersByType(eEventArrayType_Hash, aEvent->userType, PR_FALSE);
  } else {
    for (PRInt32 i = 0; i < eEventArrayType_Hash; ++i) {
      typeData = &sEventTypes[i];
      for (PRInt32 j = 0; j < typeData->numEvents; ++j) {
        dispData = &(typeData->events[j]);
        if (aEvent->message == dispData->message) {
          listeners = GetListenersByType((EventArrayType)i, nsnull, PR_FALSE);
          goto found;
        }
      }
    }
  }

 found:
  if (listeners) {
    if (!*aDOMEvent) {
      ret = CreateEvent(aPresContext, aEvent, EmptyString(), aDOMEvent);
    }

    if (NS_SUCCEEDED(ret)) {
      PRInt32 count = listeners->Count();
      nsVoidArray originalListeners(count);
      originalListeners = *listeners;

      nsAutoPopupStatePusher popupStatePusher(nsDOMEvent::GetEventPopupControlState(aEvent));

      for (PRInt32 k = 0; !mListenersRemoved && listeners && k < count; ++k) {
        nsListenerStruct* ls = NS_STATIC_CAST(nsListenerStruct*, originalListeners.FastElementAt(k));
        // Don't fire the listener if it's been removed

        if (listeners->IndexOf(ls) != -1 && ls->mFlags & aFlags &&
            ls->mGroupFlags == currentGroup &&
            (NS_IS_TRUSTED_EVENT(aEvent) ||
             ls->mFlags & NS_PRIV_EVENT_UNTRUSTED_PERMITTED)) {
          // Try the type-specific listener interface
          PRBool hasInterface = PR_FALSE;
          if (typeData)
            DispatchToInterface(*aDOMEvent, ls->mListener,
                                dispData->method, *typeData->iid,
                                &hasInterface);

          // If it doesn't implement that, call the generic HandleEvent()
          if (!hasInterface && (ls->mSubType == NS_EVENT_BITS_NONE ||
                                ls->mSubType & dispData->bits)) {
            HandleEventSubType(ls, *aDOMEvent, aCurrentTarget,
                               dispData ? dispData->bits : NS_EVENT_BITS_NONE,
                               aFlags);
          }
        }
      }
    }
  }

  if (aEvent->flags & NS_EVENT_FLAG_NO_DEFAULT) {
    *aEventStatus = nsEventStatus_eConsumeNoDefault;
  }

  return NS_OK;
}

/**
* Creates a DOM event
*/

NS_IMETHODIMP
nsEventListenerManager::CreateEvent(nsPresContext* aPresContext,
                                    nsEvent* aEvent,
                                    const nsAString& aEventType,
                                    nsIDOMEvent** aDOMEvent)
{
  *aDOMEvent = nsnull;

  if (aEvent) {
    switch(aEvent->eventStructType) {
    case NS_MUTATION_EVENT:
      return NS_NewDOMMutationEvent(aDOMEvent, aPresContext,
                                    NS_STATIC_CAST(nsMutationEvent*,aEvent));
    case NS_GUI_EVENT:
    case NS_COMPOSITION_EVENT:
    case NS_RECONVERSION_EVENT:
    case NS_QUERYCARETRECT_EVENT:
    case NS_SCROLLPORT_EVENT:
      return NS_NewDOMUIEvent(aDOMEvent, aPresContext,
                              NS_STATIC_CAST(nsGUIEvent*,aEvent));
    case NS_KEY_EVENT:
      return NS_NewDOMKeyboardEvent(aDOMEvent, aPresContext,
                                    NS_STATIC_CAST(nsKeyEvent*,aEvent));
    case NS_MOUSE_EVENT:
    case NS_MOUSE_SCROLL_EVENT:
    case NS_POPUP_EVENT:
      return NS_NewDOMMouseEvent(aDOMEvent, aPresContext,
                                 NS_STATIC_CAST(nsInputEvent*,aEvent));
    case NS_POPUPBLOCKED_EVENT:
      return NS_NewDOMPopupBlockedEvent(aDOMEvent, aPresContext,
                                        NS_STATIC_CAST(nsPopupBlockedEvent*,
                                                       aEvent));
    case NS_TEXT_EVENT:
      return NS_NewDOMTextEvent(aDOMEvent, aPresContext,
                                NS_STATIC_CAST(nsTextEvent*,aEvent));
    case NS_BEFORE_PAGE_UNLOAD_EVENT:
      return
        NS_NewDOMBeforeUnloadEvent(aDOMEvent, aPresContext,
                                   NS_STATIC_CAST(nsBeforePageUnloadEvent*,
                                                  aEvent));
    case NS_PAGETRANSITION_EVENT:
      return NS_NewDOMPageTransitionEvent(aDOMEvent, aPresContext,
                                          NS_STATIC_CAST(nsPageTransitionEvent*,
                                                         aEvent));
#ifdef MOZ_SVG
    case NS_SVG_EVENT:
      return NS_NewDOMSVGEvent(aDOMEvent, aPresContext,
                               aEvent);
    case NS_SVGZOOM_EVENT:
      return NS_NewDOMSVGZoomEvent(aDOMEvent, aPresContext,
                                   NS_STATIC_CAST(nsGUIEvent*,aEvent));
#endif // MOZ_SVG
    }

    // For all other types of events, create a vanilla event object.
    return NS_NewDOMEvent(aDOMEvent, aPresContext, aEvent);
  }

  // And if we didn't get an event, check the type argument.

  if (aEventType.LowerCaseEqualsLiteral("mouseevent") ||
      aEventType.LowerCaseEqualsLiteral("mouseevents") ||
      aEventType.LowerCaseEqualsLiteral("mousescrollevents") ||
      aEventType.LowerCaseEqualsLiteral("popupevents"))
    return NS_NewDOMMouseEvent(aDOMEvent, aPresContext,
                               NS_STATIC_CAST(nsInputEvent*,aEvent));
  if (aEventType.LowerCaseEqualsLiteral("keyboardevent") ||
      aEventType.LowerCaseEqualsLiteral("keyevents"))
    return NS_NewDOMKeyboardEvent(aDOMEvent, aPresContext,
                                  NS_STATIC_CAST(nsKeyEvent*,aEvent));
  if (aEventType.LowerCaseEqualsLiteral("mutationevent") ||
        aEventType.LowerCaseEqualsLiteral("mutationevents"))
    return NS_NewDOMMutationEvent(aDOMEvent, aPresContext,
                                  NS_STATIC_CAST(nsMutationEvent*,aEvent));
  if (aEventType.LowerCaseEqualsLiteral("textevent") ||
      aEventType.LowerCaseEqualsLiteral("textevents"))
    return NS_NewDOMTextEvent(aDOMEvent, aPresContext,
                              NS_STATIC_CAST(nsTextEvent*,aEvent));
  if (aEventType.LowerCaseEqualsLiteral("popupblockedevents"))
    return NS_NewDOMPopupBlockedEvent(aDOMEvent, aPresContext,
                                      NS_STATIC_CAST(nsPopupBlockedEvent*,
                                                     aEvent));
  if (aEventType.LowerCaseEqualsLiteral("uievent") ||
      aEventType.LowerCaseEqualsLiteral("uievents"))
    return NS_NewDOMUIEvent(aDOMEvent, aPresContext,
                            NS_STATIC_CAST(nsGUIEvent*,aEvent));
  if (aEventType.LowerCaseEqualsLiteral("event") ||
      aEventType.LowerCaseEqualsLiteral("events") ||
      aEventType.LowerCaseEqualsLiteral("htmlevents"))
    return NS_NewDOMEvent(aDOMEvent, aPresContext, aEvent);
#ifdef MOZ_SVG
  if (aEventType.LowerCaseEqualsLiteral("svgevent") ||
      aEventType.LowerCaseEqualsLiteral("svgevents"))
    return NS_NewDOMSVGEvent(aDOMEvent, aPresContext,
                             aEvent);
  if (aEventType.LowerCaseEqualsLiteral("svgzoomevent") ||
      aEventType.LowerCaseEqualsLiteral("svgzoomevents"))
    return NS_NewDOMSVGZoomEvent(aDOMEvent, aPresContext,
                                 NS_STATIC_CAST(nsGUIEvent*,aEvent));
#endif // MOZ_SVG

  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

/**
* Captures all events designated for descendant objects at the current level.
* @param an event listener
*/

NS_IMETHODIMP
nsEventListenerManager::CaptureEvent(PRInt32 aEventTypes)
{
  return FlipCaptureBit(aEventTypes, PR_TRUE);
}             

/**
* Releases all events designated for descendant objects at the current level.
* @param an event listener
*/

NS_IMETHODIMP
nsEventListenerManager::ReleaseEvent(PRInt32 aEventTypes)
{
  return FlipCaptureBit(aEventTypes, PR_FALSE);
}

nsresult
nsEventListenerManager::FlipCaptureBit(PRInt32 aEventTypes,
                                       PRBool aInitCapture)
{
  // This method exists for Netscape 4.x event handling compatibility.
  // New events do not need to be added here.

  EventArrayType arrayType = eEventArrayType_None;
  PRUint8 bits = 0;

  if (aEventTypes & nsIDOMNSEvent::MOUSEDOWN) {
    arrayType = eEventArrayType_Mouse;
    bits = NS_EVENT_BITS_MOUSE_MOUSEDOWN; 
  }
  if (aEventTypes & nsIDOMNSEvent::MOUSEUP) {
    arrayType = eEventArrayType_Mouse;
    bits = NS_EVENT_BITS_MOUSE_MOUSEUP; 
  }
  if (aEventTypes & nsIDOMNSEvent::MOUSEOVER) {
    arrayType = eEventArrayType_Mouse;
    bits = NS_EVENT_BITS_MOUSE_MOUSEOVER; 
  }
  if (aEventTypes & nsIDOMNSEvent::MOUSEOUT) {
    arrayType = eEventArrayType_Mouse;
    bits = NS_EVENT_BITS_MOUSE_MOUSEOUT; 
  }
  if (aEventTypes & nsIDOMNSEvent::MOUSEMOVE) {
    arrayType = eEventArrayType_MouseMotion;
    bits = NS_EVENT_BITS_MOUSEMOTION_MOUSEMOVE; 
  }
  if (aEventTypes & nsIDOMNSEvent::CLICK) {
    arrayType = eEventArrayType_Mouse;
    bits = NS_EVENT_BITS_MOUSE_CLICK; 
  }
  if (aEventTypes & nsIDOMNSEvent::DBLCLICK) {
    arrayType = eEventArrayType_Mouse;
    bits = NS_EVENT_BITS_MOUSE_DBLCLICK; 
  }
  if (aEventTypes & nsIDOMNSEvent::KEYDOWN) {
    arrayType = eEventArrayType_Key;
    bits = NS_EVENT_BITS_KEY_KEYDOWN; 
  }
  if (aEventTypes & nsIDOMNSEvent::KEYUP) {
    arrayType = eEventArrayType_Key;
    bits = NS_EVENT_BITS_KEY_KEYUP; 
  }
  if (aEventTypes & nsIDOMNSEvent::KEYPRESS) {
    arrayType = eEventArrayType_Key;
    bits = NS_EVENT_BITS_KEY_KEYPRESS; 
  }
  if (aEventTypes & nsIDOMNSEvent::DRAGDROP) {
    arrayType = eEventArrayType_Drag;
    bits = NS_EVENT_BITS_DRAG_ENTER; 
  }
  /*if (aEventTypes & nsIDOMNSEvent::MOUSEDRAG) {
    arrayType = kIDOMMouseListenerarrayType;
    bits = NS_EVENT_BITS_MOUSE_MOUSEDOWN; 
  }*/
  if (aEventTypes & nsIDOMNSEvent::FOCUS) {
    arrayType = eEventArrayType_Focus;
    bits = NS_EVENT_BITS_FOCUS_FOCUS; 
  }
  if (aEventTypes & nsIDOMNSEvent::BLUR) {
    arrayType = eEventArrayType_Focus;
    bits = NS_EVENT_BITS_FOCUS_BLUR; 
  }
  if (aEventTypes & nsIDOMNSEvent::SELECT) {
    arrayType = eEventArrayType_Form;
    bits = NS_EVENT_BITS_FORM_SELECT; 
  }
  if (aEventTypes & nsIDOMNSEvent::CHANGE) {
    arrayType = eEventArrayType_Form;
    bits = NS_EVENT_BITS_FORM_CHANGE; 
  }
  if (aEventTypes & nsIDOMNSEvent::RESET) {
    arrayType = eEventArrayType_Form;
    bits = NS_EVENT_BITS_FORM_RESET; 
  }
  if (aEventTypes & nsIDOMNSEvent::SUBMIT) {
    arrayType = eEventArrayType_Form;
    bits = NS_EVENT_BITS_FORM_SUBMIT; 
  }
  if (aEventTypes & nsIDOMNSEvent::LOAD) {
    arrayType = eEventArrayType_Load;
    bits = NS_EVENT_BITS_LOAD_LOAD; 
  }
  if (aEventTypes & nsIDOMNSEvent::UNLOAD) {
    arrayType = eEventArrayType_Load;
    bits = NS_EVENT_BITS_LOAD_UNLOAD; 
  }
  if (aEventTypes & nsIDOMNSEvent::ABORT) {
    arrayType = eEventArrayType_Load;
    bits = NS_EVENT_BITS_LOAD_ABORT; 
  }
  if (aEventTypes & nsIDOMNSEvent::ERROR) {
    arrayType = eEventArrayType_Load;
    bits = NS_EVENT_BITS_LOAD_ERROR; 
  }
  if (aEventTypes & nsIDOMNSEvent::RESIZE) {
    arrayType = eEventArrayType_Paint;
    bits = NS_EVENT_BITS_PAINT_RESIZE; 
  }
  if (aEventTypes & nsIDOMNSEvent::SCROLL) {
    arrayType = eEventArrayType_Scroll;
    bits = NS_EVENT_BITS_PAINT_RESIZE; 
  }

  if (arrayType != eEventArrayType_None) {
    nsListenerStruct *ls = FindJSEventListener(arrayType);

    if (ls) {
      if (aInitCapture)
        ls->mSubTypeCapture |= bits;
      else
        ls->mSubTypeCapture &= ~bits;

      ls->mFlags |= NS_EVENT_FLAG_CAPTURE;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEventListenerManager::SetListenerTarget(nsISupports* aTarget)
{
  //WEAK reference, must be set back to nsnull when done
  mTarget = aTarget;
  return NS_OK;
}

NS_IMETHODIMP
nsEventListenerManager::GetSystemEventGroupLM(nsIDOMEventGroup **aGroup)
{
  if (!gSystemEventGroup) {
    nsresult result;
    nsCOMPtr<nsIDOMEventGroup> group(do_CreateInstance(kDOMEventGroupCID,&result));
    if (NS_FAILED(result))
      return result;

    gSystemEventGroup = group;
    NS_ADDREF(gSystemEventGroup);
  }

  *aGroup = gSystemEventGroup;
  NS_ADDREF(*aGroup);
  return NS_OK;
}

nsresult
nsEventListenerManager::GetDOM2EventGroup(nsIDOMEventGroup **aGroup)
{
  if (!gDOM2EventGroup) {
    nsresult result;
    nsCOMPtr<nsIDOMEventGroup> group(do_CreateInstance(kDOMEventGroupCID,&result));
    if (NS_FAILED(result))
      return result;

    gDOM2EventGroup = group;
    NS_ADDREF(gDOM2EventGroup);
  }

  *aGroup = gDOM2EventGroup;
  NS_ADDREF(*aGroup);
  return NS_OK;
}

// nsIDOMEventTarget interface
NS_IMETHODIMP 
nsEventListenerManager::AddEventListener(const nsAString& aType, 
                                         nsIDOMEventListener* aListener, 
                                         PRBool aUseCapture)
{
  PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

  return AddEventListenerByType(aListener, aType, flags, nsnull);
}

NS_IMETHODIMP 
nsEventListenerManager::RemoveEventListener(const nsAString& aType, 
                                            nsIDOMEventListener* aListener, 
                                            PRBool aUseCapture)
{
  PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;
  
  return RemoveEventListenerByType(aListener, aType, flags, nsnull);
}

NS_IMETHODIMP
nsEventListenerManager::DispatchEvent(nsIDOMEvent* aEvent, PRBool *_retval)
{
  nsCOMPtr<nsIContent> targetContent(do_QueryInterface(mTarget));
  if (!targetContent) {
    // nothing to dispatch on -- bad!
    return NS_ERROR_FAILURE;
  }
  
  nsCOMPtr<nsIDocument> document = targetContent->GetOwnerDoc();

  // Do nothing if the element does not belong to a document
  if (!document) {
    return NS_OK;
  }

  // Obtain a presentation shell
  nsIPresShell *shell = document->GetShellAt(0);
  if (!shell) {
    return NS_OK;
  }

  nsCOMPtr<nsPresContext> context = shell->GetPresContext();

  return context->EventStateManager()->
    DispatchNewEvent(mTarget, aEvent, _retval);
}

// nsIDOM3EventTarget interface
NS_IMETHODIMP 
nsEventListenerManager::AddGroupedEventListener(const nsAString& aType, 
                                                nsIDOMEventListener* aListener, 
                                                PRBool aUseCapture,
                                                nsIDOMEventGroup* aEvtGrp)
{
  PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

  return AddEventListenerByType(aListener, aType, flags, aEvtGrp);
}

NS_IMETHODIMP 
nsEventListenerManager::RemoveGroupedEventListener(const nsAString& aType, 
                                            nsIDOMEventListener* aListener, 
                                            PRBool aUseCapture,
                                            nsIDOMEventGroup* aEvtGrp)
{
  PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;
  
  return RemoveEventListenerByType(aListener, aType, flags, aEvtGrp);
}

NS_IMETHODIMP
nsEventListenerManager::CanTrigger(const nsAString & type, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsEventListenerManager::IsRegisteredHere(const nsAString & type, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

// nsIDOMEventReceiver interface
NS_IMETHODIMP 
nsEventListenerManager::AddEventListenerByIID(nsIDOMEventListener *aListener, 
                                              const nsIID& aIID)
{
  return AddEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
}

NS_IMETHODIMP 
nsEventListenerManager::RemoveEventListenerByIID(nsIDOMEventListener *aListener, const nsIID& aIID)
{
  return RemoveEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
}

NS_IMETHODIMP 
nsEventListenerManager::GetListenerManager(nsIEventListenerManager** aInstancePtrResult)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);
  *aInstancePtrResult = NS_STATIC_CAST(nsIEventListenerManager*, this);
  NS_ADDREF(*aInstancePtrResult);
  return NS_OK;
}
 
NS_IMETHODIMP 
nsEventListenerManager::HandleEvent(nsIDOMEvent *aEvent)
{
  PRBool defaultActionEnabled;
  return DispatchEvent(aEvent, &defaultActionEnabled);
}

NS_IMETHODIMP
nsEventListenerManager::GetSystemEventGroup(nsIDOMEventGroup **aGroup)
{
  return GetSystemEventGroupLM(aGroup);
}

nsresult
nsEventListenerManager::FixContextMenuEvent(nsPresContext* aPresContext,
                                            nsIDOMEventTarget* aCurrentTarget,
                                            nsEvent* aEvent,
                                            nsIDOMEvent** aDOMEvent)
{
  // If we're here because of the key-equiv for showing context menus, we
  // have to reset the event target to the currently focused element. Get it
  // from the focus controller.
  nsCOMPtr<nsIDOMEventTarget> currentTarget(aCurrentTarget);
  nsCOMPtr<nsIDOMElement> currentFocus;
  nsIPresShell* shell = aPresContext->GetPresShell();
  if (!shell) {
    // Nothing to do.
    return NS_OK;
  }


  if (aEvent->message == NS_CONTEXTMENU_KEY) {
    nsIDocument *doc = shell->GetDocument();
    if (doc) {
      nsCOMPtr<nsPIDOMWindow> privWindow = do_QueryInterface(doc->GetScriptGlobalObject());
      if (privWindow) {
        nsIFocusController *focusController =
          privWindow->GetRootFocusController();
        if (focusController)
          focusController->GetFocusedElement(getter_AddRefs(currentFocus));
      }
    }
  }

  nsresult ret = NS_OK;

  if (nsnull == *aDOMEvent) {        
    // If we're here because of the key-equiv for showing context menus, we
    // have to twiddle with the NS event to make sure the context menu comes
    // up in the upper left of the relevant content area before we create
    // the DOM event. Since we never call InitMouseEvent() on the event, 
    // the client X/Y will be 0,0. We can make use of that if the widget is null.
    if (aEvent->message == NS_CONTEXTMENU_KEY)
      NS_IF_RELEASE(((nsGUIEvent*)aEvent)->widget);   // nulls out widget
    ret = NS_NewDOMMouseEvent(aDOMEvent, aPresContext, NS_STATIC_CAST(nsInputEvent*, aEvent));
  }

  if (NS_SUCCEEDED(ret)) {
    // update the target
    if (currentFocus) {
      // Reset event coordinates relative to focused frame in view
      nsPoint targetPt;
      GetCoordinatesFor(currentFocus, aPresContext, shell, targetPt);
      aEvent->point.x += targetPt.x - aEvent->refPoint.x;
      aEvent->point.y += targetPt.y - aEvent->refPoint.y;
      aEvent->refPoint.x = targetPt.x;
      aEvent->refPoint.y = targetPt.y;

      currentTarget = do_QueryInterface(currentFocus);
      nsCOMPtr<nsIPrivateDOMEvent> pEvent(do_QueryInterface(*aDOMEvent));
      pEvent->SetTarget(currentTarget);
    }
  }

  return ret;
}

// Get coordinates relative to root view for element, 
// first ensuring the element is onscreen
void
nsEventListenerManager::GetCoordinatesFor(nsIDOMElement *aCurrentEl, 
                                          nsPresContext *aPresContext,
                                          nsIPresShell *aPresShell, 
                                          nsPoint& aTargetPt)
{
  nsCOMPtr<nsIContent> focusedContent(do_QueryInterface(aCurrentEl));
  nsIFrame *frame = nsnull;
  aPresShell->GetPrimaryFrameFor(focusedContent, &frame);
  if (frame) {
    aPresShell->ScrollFrameIntoView(frame, NS_PRESSHELL_SCROLL_ANYWHERE,
                                           NS_PRESSHELL_SCROLL_ANYWHERE);

    nsPoint frameOrigin(0, 0);

    // Get the frame's origin within its view
    nsIView *view = frame->GetClosestView(&frameOrigin);
    NS_ASSERTION(view, "No view for frame");

    nsIViewManager* vm = aPresShell->GetViewManager();
    nsIView *rootView = nsnull;
    vm->GetRootView(rootView);
    NS_ASSERTION(rootView, "No root view in pres shell");

    // View's origin within its root view
    frameOrigin += view->GetOffsetTo(rootView);

    // Start context menu down and to the right from top left of frame
    // use the lineheight. This is a good distance to move the context
    // menu away from the top left corner of the frame. If we always 
    // used the frame height, the context menu could end up far away,
    // for example when we're focused on linked images.
    // On the other hand, we want to use the frame height if it's less
    // than the current line height, so that the context menu appears
    // associated with the correct frame.
    nscoord extra = frame->GetSize().height;
    nsIScrollableView *scrollView =
      nsLayoutUtils::GetNearestScrollingView(view, nsLayoutUtils::eEither);
    if (scrollView) {
      nscoord scrollViewLineHeight;
      scrollView->GetLineHeight(&scrollViewLineHeight);
      if (extra > scrollViewLineHeight) {
        extra = scrollViewLineHeight; 
      }
    }

    PRInt32 extraPixelsY = 0;
#ifdef MOZ_XUL
    // Tree view special case (tree items have no frames)
    // Get the focused row and add its coordinates, which are already in pixels
    // XXX Boris, should we create a new interface so that event listener manager doesn't
    // need to know about trees? Something like nsINodelessChildCreator which
    // could provide the current focus coordinates?
    nsCOMPtr<nsIDOMXULElement> xulElement(do_QueryInterface(aCurrentEl));
    if (xulElement) {
      nsCOMPtr<nsIBoxObject> box;
      xulElement->GetBoxObject(getter_AddRefs(box));
      nsCOMPtr<nsITreeBoxObject> treeBox(do_QueryInterface(box));
      if (treeBox) {
        // Factor in focused row
        nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelect =
          do_QueryInterface(aCurrentEl);
        NS_ASSERTION(multiSelect, "No multi select interface for tree");

        PRInt32 currentIndex;
        multiSelect->GetCurrentIndex(&currentIndex);
        if (currentIndex >= 0) {
          treeBox->EnsureRowIsVisible(currentIndex);
          PRInt32 firstVisibleRow, rowHeight;
          treeBox->GetFirstVisibleRow(&firstVisibleRow);
          treeBox->GetRowHeight(&rowHeight);
          extraPixelsY = (currentIndex - firstVisibleRow + 1) * rowHeight;
          extra = 0;

          nsCOMPtr<nsITreeColumns> cols;
          treeBox->GetColumns(getter_AddRefs(cols));
          if (cols) {
            nsCOMPtr<nsITreeColumn> col;
            cols->GetFirstColumn(getter_AddRefs(col));
            if (col) {
              nsCOMPtr<nsIDOMElement> colElement;
              col->GetElement(getter_AddRefs(colElement));
              nsCOMPtr<nsIContent> colContent(do_QueryInterface(colElement));
              if (colContent) {
                aPresShell->GetPrimaryFrameFor(colContent, &frame);
                if (frame) {
                  frameOrigin.y += frame->GetSize().height;
                }
              }
            }
          }
        }
      }
    }
#endif

    // Convert from twips to pixels
    float t2p = aPresContext->TwipsToPixels();
    aTargetPt.x = NSTwipsToIntPixels(frameOrigin.x + extra, t2p);
    aTargetPt.y = NSTwipsToIntPixels(frameOrigin.y + extra, t2p) + extraPixelsY;
  }
}

PRBool
nsEventListenerManager::HasUnloadListeners()
{
  nsVoidArray *listeners = GetListenersByType(eEventArrayType_Load, nsnull,
                                              PR_FALSE);
  if (listeners) {
    PRInt32 count = listeners->Count();
    for (PRInt32 i = 0; i < count; ++i) {
      PRUint32 subtype = NS_STATIC_CAST(nsListenerStruct*,
                                        listeners->FastElementAt(i))->mSubType;
      if (subtype == NS_EVENT_BITS_NONE ||
          subtype & (NS_EVENT_BITS_LOAD_UNLOAD |
                     NS_EVENT_BITS_LOAD_BEFORE_UNLOAD))
        return PR_TRUE;
    }
  }

  return PR_FALSE;
}

nsresult
NS_NewEventListenerManager(nsIEventListenerManager** aInstancePtrResult) 
{
  nsIEventListenerManager* l = new nsEventListenerManager();

  if (!l) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  return CallQueryInterface(l, aInstancePtrResult);
}

