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
 * The Original Code is Chimera code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   David Hyatt  <hyatt@mozilla.org>
 *   Simon Fraser <sfraser@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#import "NSString+Utils.h"

#import "ChimeraUIConstants.h"

#include "SaveHeaderSniffer.h"

#include "netCore.h"
#include "nsNetUtil.h"

#include "nsIChannel.h"
#include "nsIHttpChannel.h"
#include "nsIURL.h"
#include "nsIPrefService.h"
#include "nsIMIMEService.h"
#include "nsIMIMEInfo.h"
#include "nsIStringEnumerator.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIMIMEHeaderParam.h"
#include "nsIDownload.h"
#include "nsIExternalHelperAppService.h"
#include "nsCExternalHandlerService.h"

const char* const persistContractID = "@mozilla.org/embedding/browser/nsWebBrowserPersist;1";

nsHeaderSniffer::nsHeaderSniffer(nsIWebBrowserPersist* aPersist, nsIFile* aFile, nsIURI* aURL,
                nsIDOMDocument* aDocument, nsIInputStream* aPostData,
                const nsAString& aSuggestedFilename, PRBool aBypassCache,
                NSView* aFilterView)
: mSniffingPersist(aPersist)
, mTmpFile(aFile)
, mURL(aURL)
, mDocument(aDocument)
, mPostData(aPostData)
, mDefaultFilename(aSuggestedFilename)
, mBypassCache(aBypassCache)
, mFilterView(aFilterView)
{
}

nsHeaderSniffer::~nsHeaderSniffer()
{
}

NS_IMPL_ISUPPORTS2(nsHeaderSniffer, nsIInterfaceRequestor, nsIWebProgressListener)

// Implementation of nsIInterfaceRequestor
NS_IMETHODIMP 
nsHeaderSniffer::GetInterface(const nsIID &aIID, void** aInstancePtr)
{
  return QueryInterface(aIID, aInstancePtr);
}

#pragma mark -

// Implementation of nsIWebProgressListener
/* void onStateChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in long aStateFlags, in unsigned long aStatus); */
NS_IMETHODIMP 
nsHeaderSniffer::OnStateChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 aStateFlags, 
                                PRUint32 aStatus)
{  
  if (aStateFlags & nsIWebProgressListener::STATE_START)
  {
    nsCOMPtr<nsIWebBrowserPersist> kungFuDeathGrip(mSniffingPersist);   // be sure to keep it alive while we save
                                                                // since it owns us as a listener
    nsCOMPtr<nsIWebProgressListener> kungFuSuicideGrip(this);   // and keep ourslves alive
    
    nsresult rv;
    nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest, &rv);
    if (!channel) return rv;
    channel->GetContentType(mContentType);
    
    nsCOMPtr<nsIURI> origURI;
    channel->GetOriginalURI(getter_AddRefs(origURI));
    
    // Get the content-disposition if we're an HTTP channel.
    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
    if (httpChannel)
      httpChannel->GetResponseHeader(nsCAutoString("content-disposition"), mContentDisposition);
    
    // Kill the web browser persist that we used to get this far. We restart the download, now
    // that we have the file name, content disposition etc.
    mSniffingPersist->CancelSave();
    mSniffingPersist = nsnull;

    PRBool exists;
    mTmpFile->Exists(&exists);
    if (exists)
        mTmpFile->Remove(PR_FALSE);

    // this will create a new nsIWebBrowserPersist, and set a new download instance
    // as its progress listener.
    rv = PerformSave(origURI);
    if (NS_FAILED(rv))
    {
      // if this failed, we should have already notified the downloader (via an OnStateChange callback)
      // that something went wrong, so no need to show more UI here.
    }
  }
  return NS_OK;
}

/* void onProgressChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in long aCurSelfProgress, in long aMaxSelfProgress, in long aCurTotalProgress, in long aMaxTotalProgress); */
NS_IMETHODIMP 
nsHeaderSniffer::OnProgressChange(nsIWebProgress *aWebProgress, 
           nsIRequest *aRequest, 
           PRInt32 aCurSelfProgress, 
           PRInt32 aMaxSelfProgress, 
           PRInt32 aCurTotalProgress, 
           PRInt32 aMaxTotalProgress)
{
  return NS_OK;
}

/* void onLocationChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in nsIURI location); */
NS_IMETHODIMP 
nsHeaderSniffer::OnLocationChange(nsIWebProgress *aWebProgress, 
           nsIRequest *aRequest, 
           nsIURI *location)
{
  return NS_OK;
}

/* void onStatusChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in nsresult aStatus, in wstring aMessage); */
NS_IMETHODIMP 
nsHeaderSniffer::OnStatusChange(nsIWebProgress *aWebProgress, 
               nsIRequest *aRequest, 
               nsresult aStatus, 
               const PRUnichar *aMessage)
{
  return NS_OK;
}

/* void onSecurityChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in unsigned long state); */
NS_IMETHODIMP 
nsHeaderSniffer::OnSecurityChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 state)
{
  return NS_OK;
}

#pragma mark -


nsresult
nsHeaderSniffer::PerformSave(nsIURI* inOriginalURI)
{
    nsresult rv;
    // Are we an HTML document? If so, we will want to append an accessory view to
    // the save dialog to provide the user with the option of doing a complete
    // save vs. a single file save.
    PRBool isHTML = (mDocument && mContentType.Equals("text/html"));
    
    // Next find out the directory that we should start in.
    nsCOMPtr<nsIPrefService> prefs(do_GetService("@mozilla.org/preferences-service;1", &rv));
    if (!prefs)
        return rv;
    nsCOMPtr<nsIPrefBranch> dirBranch;
    prefs->GetBranch("browser.download.", getter_AddRefs(dirBranch));
    PRInt32 filterIndex = eSaveFormatHTMLComplete;
    if (dirBranch) {
        rv = dirBranch->GetIntPref("save_converter_index", &filterIndex);
        if (NS_FAILED(rv))
            filterIndex = eSaveFormatHTMLComplete;
    }
    
    NSPopUpButton* filterList = [mFilterView viewWithTag:kSaveFormatPopupTag];
    if (filterList)
        [filterList selectItemAtIndex: filterIndex];
        
    // We need to figure out what file name to use.
    nsAutoString defaultFileName;
    if (!mContentDisposition.IsEmpty()) {
        // (1) Use the HTTP header suggestion.
        nsCAutoString fallbackCharset;
        nsCOMPtr<nsIMIMEHeaderParam> mimehdrpar = 
            do_GetService("@mozilla.org/network/mime-hdrparam;1");
        if (mimehdrpar) {
            if (mURL)
                mURL->GetOriginCharset(fallbackCharset);
            rv = mimehdrpar->GetParameter(mContentDisposition, "filename",
                                          fallbackCharset, PR_TRUE, nsnull,
                                          defaultFileName);
            if (NS_FAILED(rv) || defaultFileName.IsEmpty())
               rv = mimehdrpar->GetParameter(mContentDisposition, "name",
                                             fallbackCharset, PR_TRUE, nsnull,
                                             defaultFileName);
        }
    }
    
    // If we're saving view-source, create a temporary URI which is
    // the original URI which the source came from and use it in the
    // steps below which divine a file name from a URI.
    PRBool isViewSrc = PR_FALSE;
    nsCOMPtr<nsIURI> origURI;
    if (mURL) {
        nsCAutoString scheme;
        mURL->GetScheme(scheme);
        if (scheme.Equals("view-source")) {
            nsCAutoString path;
            rv = mURL->GetPath(path);
            if (NS_FAILED(rv))
                return rv;
            rv = NS_NewURI(getter_AddRefs(origURI), path);
            if (NS_FAILED(rv))
                return rv;
            isViewSrc = PR_TRUE;
        }
        else
            origURI = mURL;
    }
    
    if (defaultFileName.IsEmpty()) {
        nsCOMPtr<nsIURL> url(do_QueryInterface(origURI));
        if (url) {
            nsCAutoString urlFileName;
            url->GetFileName(urlFileName); // (2) For file URLs, use the file name.
            NSString* unescapedString = [NSString unescapedURLString:[NSString stringWithUTF8String:urlFileName.get()]];
            CopyUTF8toUTF16([unescapedString UTF8String], defaultFileName);
        }
    }
    
    if (defaultFileName.IsEmpty() && mDocument && isHTML) {
        nsCOMPtr<nsIDOMHTMLDocument> htmlDoc(do_QueryInterface(mDocument));
        nsAutoString title;
        if (htmlDoc)
            htmlDoc->GetTitle(title); // (3) Use the title of the document.
        defaultFileName = title;
    }
    
    if (defaultFileName.IsEmpty()) {
        // (4) Use the caller provided name.
        defaultFileName = mDefaultFilename;
    }

    if (defaultFileName.IsEmpty() && origURI) {
        // (5) Use the host.
        nsCAutoString hostName;
        origURI->GetHost(hostName);
        CopyUTF8toUTF16(hostName, defaultFileName);
    }
    
    // One last case to handle about:blank and other fruity untitled pages.
    if (defaultFileName.IsEmpty())
        defaultFileName = NS_LITERAL_STRING("untitled");		// XXX localize
        
    // Validate the file name to ensure legality.
    for (PRUint32 i = 0; i < defaultFileName.Length(); i++)
        if (defaultFileName[i] == ':' || defaultFileName[i] == '/')
            defaultFileName.SetCharAt(' ', i);
            
    // Make sure the appropriate extension is appended to the suggested file name.
    nsCOMPtr<nsIURI> fileURI(do_CreateInstance("@mozilla.org/network/standard-url;1"));
    nsCOMPtr<nsIURL> fileURL(do_QueryInterface(fileURI, &rv));
    if (!fileURL)
        return rv;
    fileURL->SetFilePath(NS_ConvertUTF16toUTF8(defaultFileName));
    
    nsCAutoString fileExtension;
    fileURL->GetFileExtension(fileExtension);
    
    PRBool setExtension = PR_FALSE;
    if (mContentType.Equals("text/html") || isViewSrc) {
        if (fileExtension.IsEmpty() || (!fileExtension.Equals("htm") && !fileExtension.Equals("html"))) {
            defaultFileName.AppendWithConversion(".html");
            setExtension = PR_TRUE;
        }
    }
    
    if (!setExtension && fileExtension.IsEmpty()) {
        nsCOMPtr<nsIMIMEService> mimeService(do_GetService("@mozilla.org/mime;1", &rv));
        if (!mimeService)
            return rv;
        nsCOMPtr<nsIMIMEInfo> mimeInfo;
        rv = mimeService->GetFromTypeAndExtension(mContentType, EmptyCString(), getter_AddRefs(mimeInfo));
        if (!mimeInfo)
          return rv;

        nsCOMPtr<nsIUTF8StringEnumerator> extensions;
        mimeInfo->GetFileExtensions(getter_AddRefs(extensions));        
        if (extensions) {
            PRBool hasMore;
            extensions->HasMore(&hasMore);
            if (hasMore) {
                nsCAutoString ext;
                extensions->GetNext(ext);
                defaultFileName += PRUnichar('.');
                AppendUTF8toUTF16(ext, defaultFileName);
            }
        }
    }
    
    // Now it's time to pose the save dialog.
    NSSavePanel* savePanel = [NSSavePanel savePanel];
    NSString* file = nil;
    if (!defaultFileName.IsEmpty())
        file = [NSString stringWith_nsAString:defaultFileName];
        
    if (isHTML) {
        [savePanel setAccessoryView: mFilterView];
        // need this because the |stringWith_nsAString| returns a NSString 
        // with an extension that the savePanel does not like and will add
        // the extensions on top of an extensions
        if (filterIndex == eSaveFormatPlainText) {
          file = [file stringByDeletingPathExtension];
          file = [file stringByAppendingPathExtension:@"txt"];
        }
        NSLog([file pathExtension]);
        [savePanel setRequiredFileType:[file pathExtension]];
        [savePanel setCanSelectHiddenExtension: YES];
    }
        
    if ([savePanel runModalForDirectory: nil file: file] == NSFileHandlingPanelCancelButton)
        return NS_OK;
       
    // Update the filter index.
    if (isHTML && filterList) {
        filterIndex = [filterList indexOfSelectedItem];
        dirBranch->SetIntPref("save_converter_index", filterIndex);
    }
    
    // Convert the content type to text/plain if it was selected in the filter.
    if (isHTML && filterIndex == eSaveFormatPlainText)
        mContentType = "text/plain";
    
    nsCOMPtr<nsISupports> sourceData;
    if (isHTML && filterIndex != eSaveFormatHTMLSource)
        sourceData = do_QueryInterface(mDocument);
    else
        sourceData = do_QueryInterface(mURL);

    nsAutoString dstString;
    [[savePanel filename] assignTo_nsAString:dstString];

    return InitiateDownload(sourceData, dstString, inOriginalURI);
}

// inOriginalURI is always a URI. inSourceData can be an nsIURI or an nsIDOMDocument, depending
// on what we're saving. It's that way for nsIWebBrowserPersist.
nsresult
nsHeaderSniffer::InitiateDownload(nsISupports* inSourceData, nsString& inFileName, nsIURI* inOriginalURI)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIWebBrowserPersist> webPersist = do_CreateInstance(persistContractID, &rv);
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr<nsIURI> sourceURI = do_QueryInterface(inSourceData);

  nsCOMPtr<nsILocalFile> destFile;
  rv = NS_NewLocalFile(inFileName, PR_FALSE, getter_AddRefs(destFile));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIURI> destURI;
  rv = NS_NewFileURI(getter_AddRefs(destURI), destFile);
  if (NS_FAILED(rv)) return rv;

  PRInt64 timeNow = PR_Now();
  
  nsCOMPtr<nsIDownload> downloader = do_CreateInstance(NS_TRANSFER_CONTRACTID);
  // dlListener attaches to its progress dialog here, which gains ownership
  rv = downloader->Init(inOriginalURI, destURI, inFileName, nsnull, timeNow,
                        nsnull, webPersist);
  if (NS_FAILED(rv)) return rv;
    
  PRInt32 flags = nsIWebBrowserPersist::PERSIST_FLAGS_REPLACE_EXISTING_FILES;
  webPersist->SetPersistFlags(flags);

  webPersist->SetProgressListener(downloader);
  
  if (sourceURI)
  {
    flags |= nsIWebBrowserPersist::PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION;
    webPersist->SetPersistFlags(flags);
    rv = webPersist->SaveURI(sourceURI, nsnull, nsnull, mPostData, nsnull, destURI);
  }
  else
  {
    nsCOMPtr<nsIDOMHTMLDocument> domDoc = do_QueryInterface(inSourceData, &rv);
    if (!domDoc) return rv;  // should never happen
    
    PRInt32 encodingFlags = 0;
    nsCOMPtr<nsIFile> filesFolder;
    
    if (!mContentType.Equals("text/plain"))
    {
      // Create a local directory in the same dir as our file.  It
      // will hold our associated files.
      destFile->Clone(getter_AddRefs(filesFolder));
      
      nsAutoString leafName;
      filesFolder->GetLeafName(leafName);
      nsAutoString nameMinusExt(leafName);
      PRInt32 index = nameMinusExt.RFind(".");
      if (index >= 0)
          nameMinusExt.Left(nameMinusExt, index);
      nameMinusExt += NS_LITERAL_STRING(" Files"); // XXXdwh needs to be localizable!
      filesFolder->SetLeafName(nameMinusExt);
      PRBool exists = PR_FALSE;
      filesFolder->Exists(&exists);
      if (!exists) {
        rv = filesFolder->Create(nsILocalFile::DIRECTORY_TYPE, 0755);
        if (NS_FAILED(rv))
        {
          // we need to send a pseudo OnStateChange to tell the download that something went wrong.
          // (nsWebProgressListener does this too)
          downloader->OnStateChange(nsnull, nsnull,
                                           nsIWebProgressListener::STATE_START,
                                           NS_OK);
          downloader->OnStateChange(nsnull, nsnull,
                                           nsIWebProgressListener::STATE_STOP,
                                           rv);
          return rv;
        }
      }
    }
    else
    {
      encodingFlags |= nsIWebBrowserPersist::ENCODE_FLAGS_FORMATTED |
                        nsIWebBrowserPersist::ENCODE_FLAGS_ABSOLUTE_LINKS |
                        nsIWebBrowserPersist::ENCODE_FLAGS_NOFRAMES_CONTENT;
    }
    rv = webPersist->SaveDocument(domDoc, destURI, filesFolder, mContentType.get(), encodingFlags, 80 /* wrap column */);
  }

  return rv;
}

#pragma mark -

@implementation FilterViewController

-(IBAction)setNewSaveOption:(id)sender
{
  if ([[mFilterView window] isKindOfClass:[NSSavePanel class]]) 
  {
    NSSavePanel* savePanel = (NSSavePanel*) [mFilterView window];
    if (savePanel) 
    { 
      if ([mSaveOptionsPopUpButton indexOfSelectedItem] == eSaveFormatPlainText) 
        [savePanel setRequiredFileType:@"txt"];
      
      else  // rest of the selections all use .html extension
        [savePanel setRequiredFileType:@"html"];
    }
  }
}

@end
