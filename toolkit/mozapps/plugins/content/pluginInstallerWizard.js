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
 * The Original Code is Plugin Finder Service.
 *
 * The Initial Developer of the Original Code is
 * IBM Corporation.
 * Portions created by the IBM Corporation are Copyright (C) 2004
 * IBM Corporation. All Rights Reserved.
 *
 * Contributor(s):
 *   Doron Rosenberg <doronr@us.ibm.com>
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

function nsPluginInstallerWizard(){

  // create the request array
  this.mPluginRequestArray = new Object();
  // since the array is a hash, store the length
  this.mPluginRequestArrayLength = 0;

  // create the plugin info array.
  // a hash indexed by plugin id so we don't install 
  // the same plugin more than once.
  this.mPluginInfoArray = new Object();
  this.mPluginInfoArrayLength = 0;

  // holds plugins we couldn't find
  this.mPluginNotFoundArray = new Object();
  this.mPluginNotFoundArrayLength = 0;

  // array holding pids of plugins that require a license
  this.mPluginLicenseArray = new Array();

  this.mTab = null;
  this.mSuccessfullPluginInstallation = 0;

  // arguments[0] is an array that contains two items:
  //     an array of mimetypes that are missing
  //     a reference to the tab that needs them, so we can reload it

  if ("arguments" in window) {
    for (var item in window.arguments[0].plugins){
      this.mPluginRequestArray[window.arguments[0].plugins[item].mimetype] =
        new nsPluginRequest(window.arguments[0].plugins[item]);

      this.mPluginRequestArrayLength++;
    }

    this.mTab = window.arguments[0].tab;
  }

  this.WSPluginCounter = 0;
  this.licenseAcceptCounter = 0;

  this.prefBranch = null;
}

nsPluginInstallerWizard.prototype.getPluginData = function (){
  // for each mPluginRequestArray item, call the datasource
  this.WSPluginCounter = 0;

  // initiate the datasource call
  var rdfUpdater = new nsRDFItemUpdater(this.getOS(), this.getChromeLocale());

  for (item in this.mPluginRequestArray) {
    rdfUpdater.checkForPlugin(this.mPluginRequestArray[item]);
  }
}

// aPluginInfo is null if the datasource call failed, and pid is -1 if
// no matching plugin was found.
nsPluginInstallerWizard.prototype.pluginInfoReceived = function (aPluginInfo){
  this.WSPluginCounter++;

  if (aPluginInfo && (aPluginInfo.pid != -1) ) {
    // hash by id
    this.mPluginInfoArray[aPluginInfo.pid] = new PluginInfo(aPluginInfo);
    this.mPluginInfoArrayLength++;
  } else {
    this.mPluginNotFoundArray[aPluginInfo.requestedMimetype] = new PluginInfo(aPluginInfo);
    this.mPluginNotFoundArrayLength++;
  }

  var progressMeter = document.getElementById("ws_request_progress");

  if (progressMeter.getAttribute("mode") == "undetermined")
    progressMeter.setAttribute("mode", "determined");

  progressMeter.setAttribute("value",
      ((this.WSPluginCounter / this.mPluginRequestArrayLength) * 100) + "%");

  if (this.WSPluginCounter == this.mPluginRequestArrayLength) {
    // check if no plugins were found
    if (this.mPluginInfoArrayLength == 0) {
      this.advancePage("lastpage", true, false, false);
    } else {
      // we want to allow user to cancel
      this.advancePage(null, true, false, true);
    }
  } else {
    // process more.
  }
}

nsPluginInstallerWizard.prototype.showPluginList = function (){
  var myPluginList = document.getElementById("pluginList");
  var hasPluginWithInstallerUI = false;

  // clear children
  for (var run = myPluginList.childNodes.length; run > 0; run--)
    myPluginList.removeChild(myPluginList.childNodes.item(run));

  for (pluginInfoItem in this.mPluginInfoArray){
    // [plugin image] [Plugin_Name Plugin_Version]

    var pluginInfo = this.mPluginInfoArray[pluginInfoItem];

    // create the checkbox
    var myCheckbox = document.createElement("checkbox");
    myCheckbox.setAttribute("checked", "true");
    myCheckbox.setAttribute("oncommand", "gPluginInstaller.toggleInstallPlugin('" + pluginInfo.pid + "', this)");
    // XXXlocalize (nit)
    myCheckbox.setAttribute("label", pluginInfo.name + " " + (pluginInfo.version ? pluginInfo.version : ""));
    myCheckbox.setAttribute("src", pluginInfo.IconUrl);

    myPluginList.appendChild(myCheckbox);

    if (pluginInfo.InstallerShowsUI == "true")
      hasPluginWithInstallerUI = true;
  }

  if (hasPluginWithInstallerUI)
    document.getElementById("installerUI").hidden = false;

  this.canAdvance(true);
  this.canRewind(false);
}

nsPluginInstallerWizard.prototype.toggleInstallPlugin = function (aPid, aCheckbox) {
  this.mPluginInfoArray[aPid].toBeInstalled = aCheckbox.checked;

  // if no plugins are checked, don't allow to advance  
  var pluginsToInstallNum = 0;
  for (pluginInfoItem in this.mPluginInfoArray){
    if (this.mPluginInfoArray[pluginInfoItem].toBeInstalled)
      pluginsToInstallNum++;
  }

  if (pluginsToInstallNum > 0)
    this.canAdvance(true);
  else
    this.canAdvance(false);
}

nsPluginInstallerWizard.prototype.canAdvance = function (aBool){
  document.getElementById("plugin-installer-wizard").canAdvance = aBool;
}

nsPluginInstallerWizard.prototype.canRewind = function (aBool){
  document.getElementById("plugin-installer-wizard").canRewind = aBool;
}

nsPluginInstallerWizard.prototype.canCancel = function (aBool){
  document.documentElement.getButton("cancel").disabled = !aBool;
}

nsPluginInstallerWizard.prototype.showLicenses = function (){
  this.canAdvance(false);
  this.canRewind(false);

  // only add if a license is provided and the plugin was selected to
  // be installed
  for (pluginInfoItem in this.mPluginInfoArray){
    var myPluginInfoItem = this.mPluginInfoArray[pluginInfoItem];
    if (myPluginInfoItem.toBeInstalled && myPluginInfoItem.licenseURL && (myPluginInfoItem.licenseURL != ""))
      this.mPluginLicenseArray.push(myPluginInfoItem.pid);
  }

  if (this.mPluginLicenseArray.length == 0) {
    // no plugins require licenses
    this.advancePage(null, true, false, false);
  } else {
    this.licenseAcceptCounter = 0;

    // add a nsIWebProgress listener to the license iframe.
    var docShell = document.getElementById("licenseIFrame").docShell;
    var iiReq = docShell.QueryInterface(Components.interfaces.nsIInterfaceRequestor);
    var webProgress = iiReq.getInterface(Components.interfaces.nsIWebProgress);
    webProgress.addProgressListener(gPluginInstaller.progressListener,
                                    Components.interfaces.nsIWebProgress.NOTIFY_ALL);

    this.showLicense();
  }
}

nsPluginInstallerWizard.prototype.enableNext = function (){
  gPluginInstaller.canAdvance(true);
  document.getElementById("licenseRadioGroup1").disabled = false;
  document.getElementById("licenseRadioGroup2").disabled = false;
}

const nsIWebProgressListener = Components.interfaces.nsIWebProgressListener;
nsPluginInstallerWizard.prototype.progressListener = {
  onStateChange : function(aWebProgress, aRequest, aStateFlags, aStatus)
  {
    if ((aStateFlags & nsIWebProgressListener.STATE_STOP) &&
       (aStateFlags & nsIWebProgressListener.STATE_IS_NETWORK)) {
      // iframe loaded
      gPluginInstaller.enableNext();
    }
  },

  onProgressChange : function(aWebProgress, aRequest, aCurSelfProgress,
                              aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress)
  {},
  onStatusChange : function(aWebProgress, aRequest, aStatus, aMessage)
  {},

  QueryInterface : function(aIID)
  {
     if (aIID.equals(Components.interfaces.nsIWebProgressListener) ||
         aIID.equals(Components.interfaces.nsISupportsWeakReference) ||
         aIID.equals(Components.interfaces.nsISupports))
       return this;
     throw Components.results.NS_NOINTERFACE;
   }
}

nsPluginInstallerWizard.prototype.showLicense = function (){
  var pluginInfo = this.mPluginInfoArray[this.mPluginLicenseArray[this.licenseAcceptCounter]];

  this.canAdvance(false);

  loadFlags = Components.interfaces.nsIWebNavigation.LOAD_FLAGS_NONE;
  document.getElementById("licenseIFrame").webNavigation.loadURI(pluginInfo.licenseURL, loadFlags, null, null, null);

  document.getElementById("pluginLicenseLabel").firstChild.nodeValue = 
    this.getFormattedString("pluginLicenseAgreement.label", [pluginInfo.name]);

  document.getElementById("licenseRadioGroup1").disabled = true;
  document.getElementById("licenseRadioGroup2").disabled = true;
  document.getElementById("licenseRadioGroup").selectedIndex = 
    pluginInfo.licenseAccepted ? 0 : 1;
}

nsPluginInstallerWizard.prototype.showNextLicense = function (){
  var rv = true;

  if (this.mPluginLicenseArray.length > 0) {
    this.storeLicenseRadioGroup();

    this.licenseAcceptCounter++;

    if (this.licenseAcceptCounter < this.mPluginLicenseArray.length) {
      this.showLicense();

      rv = false;
      this.canRewind(true);
    }
  }

  return rv;
}

nsPluginInstallerWizard.prototype.showPreviousLicense = function (){
  this.storeLicenseRadioGroup();
  this.licenseAcceptCounter--;

  if (this.licenseAcceptCounter > 0)
    this.canRewind(true);
  else
    this.canRewind(false);

  this.showLicense();
  
  // don't allow to return from the license screens
  return false;
}

nsPluginInstallerWizard.prototype.storeLicenseRadioGroup = function (){
  var pluginInfo = this.mPluginInfoArray[this.mPluginLicenseArray[this.licenseAcceptCounter]];
  pluginInfo.licenseAccepted = !document.getElementById("licenseRadioGroup").selectedIndex;
}

nsPluginInstallerWizard.prototype.advancePage = function (aPageId, aCanAdvance, aCanRewind, aCanCancel){
  this.canAdvance(true);
  document.getElementById("plugin-installer-wizard").advance(aPageId);

  this.canAdvance(aCanAdvance);
  this.canRewind(aCanRewind);
  this.canCancel(aCanCancel);
}

nsPluginInstallerWizard.prototype.startPluginInstallation = function (){
  this.canAdvance(false);
  this.canRewind(false);

  // since the user can choose what plugins to install, we need to store
  // which ones were choosen, as nsIXPInstallManager returns an index and not the
  // mimetype.  So store the pids.

  var pluginURLArray = new Array();
  var pluginHashArray = new Array();
  var pluginPidArray = new Array();

  for (pluginInfoItem in this.mPluginInfoArray){
    var pluginItem = this.mPluginInfoArray[pluginInfoItem];

    // only push to the array if it has an XPILocation, else nsIXPInstallManager
    // will complain.
    if (pluginItem.toBeInstalled && pluginItem.XPILocation && pluginItem.licenseAccepted) {
      pluginURLArray.push(pluginItem.XPILocation);
      pluginHashArray.push(pluginItem.XPIHash);
      pluginPidArray.push(pluginItem.pid);
    }
  }

  if (pluginURLArray.length > 0)
    PluginInstallService.startPluginInsallation(pluginURLArray,
                                                pluginHashArray,
                                                pluginPidArray);
  else
    this.advancePage(null, true, false, false);
}

/*
  0    starting download
  1    download finished
  2    starting installation
  3    finished installation
  4    all done
*/
nsPluginInstallerWizard.prototype.pluginInstallationProgress = function (aPid, aProgress, aError) {

  var statMsg = null;
  var pluginInfo = gPluginInstaller.mPluginInfoArray[aPid];

  switch (aProgress) {

    case 0:
      statMsg = this.getFormattedString("pluginInstallation.download.start", [pluginInfo.name]);
      break;

    case 1:
      statMsg = this.getFormattedString("pluginInstallation.download.finish", [pluginInfo.name]);
      break;

    case 2:
      statMsg = this.getFormattedString("pluginInstallation.install.start", [pluginInfo.name]);
      break;

    case 3:
      if (aError) {
        statMsg = this.getFormattedString("pluginInstallation.install.error", [pluginInfo.name, aError]);
        pluginInfo.error = aError;
      } else {
        statMsg = this.getFormattedString("pluginInstallation.install.finish", [pluginInfo.name]);
        pluginInfo.error = null;
      }
      break;

    case 4:
      statMsg = this.getString("pluginInstallation.complete");
      break;
  }

  if (statMsg)
    document.getElementById("plugin_install_progress_message").value = statMsg;

  if (aProgress == 4) {
    this.advancePage(null, true, false, false);
  }
}

nsPluginInstallerWizard.prototype.pluginInstallationProgressMeter = function (aPid, aValue, aMaxValue){
  var progressElm = document.getElementById("plugin_install_progress");

  if (progressElm.getAttribute("mode") == "undetermined")
    progressElm.setAttribute("mode", "determined");
  
  progressElm.setAttribute("value", Math.ceil((aValue / aMaxValue) * 100) + "%")
}

nsPluginInstallerWizard.prototype.addPluginResultRow = function (aImgSrc, aName, aNameTooltip, aStatus, aStatusTooltip, aManualUrl){
  var myRows = document.getElementById("pluginResultList");

  var myRow = document.createElement("row");
  myRow.setAttribute("align", "center");

  // create the image
  var myImage = document.createElement("image");
  myImage.setAttribute("src", aImgSrc);
  myImage.setAttribute("height", "16px");
  myImage.setAttribute("width", "16px");
  myRow.appendChild(myImage)

  // create the labels
  var myLabel = document.createElement("label");
  myLabel.setAttribute("value", aName);
  if (aNameTooltip)
    myLabel.setAttribute("tooltiptext", aNameTooltip);
  myRow.appendChild(myLabel);

  if (aStatus) {
    myLabel = document.createElement("label");
    myLabel.setAttribute("value", aStatus);
    myRow.appendChild(myLabel);
  }

  // manual install
  if (aManualUrl) {
    var myButton = document.createElement("button");

    var manualInstallLabel = this.getString("pluginInstallationSummary.manualInstall.label");
    var manualInstallTooltip = this.getString("pluginInstallationSummary.manualInstall.tooltip");

    myButton.setAttribute("label", manualInstallLabel);
    myButton.setAttribute("tooltiptext", manualInstallTooltip);

    myRow.appendChild(myButton);

    // XXX: XUL sucks, need to add the listener after it got added into the document
    if (aManualUrl)
      myButton.addEventListener("command", function() { gPluginInstaller.loadURL(aManualUrl) }, false);
  }

  myRows.appendChild(myRow);
}

nsPluginInstallerWizard.prototype.showPluginResults = function (){
  var notInstalledList = "?action=missingplugins";
  var needsRestart = false;
  var myRows = document.getElementById("pluginResultList");

  // clear children
  for (var run = myRows.childNodes.length; run--; run > 0)
    myRows.removeChild(myRows.childNodes.item(run));

  for (pluginInfoItem in this.mPluginInfoArray){
    // [plugin image] [Plugin_Name Plugin_Version] [Success/Failed] [Manual Install (if Failed)]

    var myPluginItem = this.mPluginInfoArray[pluginInfoItem];

    if (myPluginItem.toBeInstalled) {
      var statusMsg;
      var statusTooltip;
      if (myPluginItem.error){
        statusMsg = this.getString("pluginInstallationSummary.failed");
        statusTooltip = myPluginItem.error;
        notInstalledList += "&mimetype=" + pluginInfoItem;
      } else if (!myPluginItem.licenseAccepted) {
        statusMsg = this.getString("pluginInstallationSummary.licenseNotAccepted");
      } else if (!myPluginItem.XPILocation) {
        statusMsg = this.getString("pluginInstallationSummary.notAvailable");
        notInstalledList += "&mimetype=" + pluginInfoItem;
      } else {
        this.mSuccessfullPluginInstallation++;
        statusMsg = this.getString("pluginInstallationSummary.success");

        // only check needsRestart if the plugin was successfully installed.
        if (myPluginItem.needsRestart)
          needsRestart = true;
      }

      // manual url - either returned from the webservice or the pluginspage attribute
      var manualUrl;
      if ((myPluginItem.error || !myPluginItem.XPILocation) && (myPluginItem.manualInstallationURL || this.mPluginRequestArray[myPluginItem.requestedMimetype].pluginsPage)){
        manualUrl = myPluginItem.manualInstallationURL ? myPluginItem.manualInstallationURL : this.mPluginRequestArray[myPluginItem.requestedMimetype].pluginsPage;
      }

      this.addPluginResultRow(
          myPluginItem.IconUrl, 
          myPluginItem.name + " " + (myPluginItem.version ? myPluginItem.version : ""),
          null,
          statusMsg, 
          statusTooltip,
          manualUrl);
    }
  }

  // handle plugins we couldn't find
  for (pluginInfoItem in this.mPluginNotFoundArray){
    var pluginRequest = this.mPluginRequestArray[pluginInfoItem];

    // if there is a pluginspage, show UI
    if (pluginRequest && pluginRequest.pluginsPage) {
      this.addPluginResultRow(
          "",
          this.getString("pluginInstallation.unknownPlugin"),
          pluginInfoItem,
          null,
          null,
          this.mPluginRequestArray[pluginInfoItem].pluginsPage);
    }

    notInstalledList += "&mimetype=" + pluginInfoItem;
  }

  // no plugins were found, so change the description of the final page.
  if (this.mPluginInfoArrayLength == 0) {
    var noPluginsFound = this.getString("pluginInstallation.noPluginsFound");
    document.getElementById("pluginSummaryDescription").setAttribute("value", noPluginsFound);
  } else if (this.mSuccessfullPluginInstallation == 0) {
    // plugins found, but none were installed.
    var noPluginsInstalled = this.getString("pluginInstallation.noPluginsInstalled");
    document.getElementById("pluginSummaryDescription").setAttribute("value", noPluginsInstalled);
  }

  document.getElementById("pluginSummaryRestartNeeded").hidden = !needsRestart;

  var app = Components.classes["@mozilla.org/xre/app-info;1"]
                      .getService(Components.interfaces.nsIXULAppInfo);

  // set the get more info link to contain the mimetypes we couldn't install.
  notInstalledList +=
    "&appID=" + app.ID +
    "&appVersion=" + app.platformBuildID +
    "&clientOS=" + this.getOS() +
    "&chromeLocale=" + this.getChromeLocale();

  document.getElementById("moreInfoLink").addEventListener("click", function() { gPluginInstaller.loadURL("https://pfs.mozilla.org/plugins/" + notInstalledList) }, false);

  this.canAdvance(true);
  this.canRewind(false);
  this.canCancel(false);
}

nsPluginInstallerWizard.prototype.loadURL = function (aUrl){
  // Check if the page where the plugin came from can load aUrl before
  // loading it, and do *not* allow loading javascript: or data: URIs.
  var pluginPage = window.opener.content.location.href;

  const nsIScriptSecurityManager =
    Components.interfaces.nsIScriptSecurityManager;
  var secMan =
    Components.classes["@mozilla.org/scriptsecuritymanager;1"]
    .getService(nsIScriptSecurityManager);

  secMan.checkLoadURIStr(pluginPage, aUrl,
                         nsIScriptSecurityManager.DISALLOW_SCRIPT_OR_DATA);

  window.opener.open(aUrl);
}

nsPluginInstallerWizard.prototype.getString = function (aName){
  return document.getElementById("pluginWizardString").getString(aName);
}

nsPluginInstallerWizard.prototype.getFormattedString = function (aName, aArray){
  return document.getElementById("pluginWizardString").getFormattedString(aName, aArray);
}

nsPluginInstallerWizard.prototype.getOS = function (){
  var httpService = Components.classes["@mozilla.org/network/protocol;1?name=http"]
                              .getService(Components.interfaces.nsIHttpProtocolHandler);
  return httpService.oscpu;
}

nsPluginInstallerWizard.prototype.getChromeLocale = function (){
  var chromeReg = Components.classes["@mozilla.org/chrome/chrome-registry;1"]
                            .getService(Components.interfaces.nsIXULChromeRegistry);
  return chromeReg.getSelectedLocale("global");
}

nsPluginInstallerWizard.prototype.getPrefBranch = function (){
  if (!this.prefBranch)
    this.prefBranch = Components.classes["@mozilla.org/preferences-service;1"]
                                .getService(Components.interfaces.nsIPrefBranch);
  return this.prefBranch;
}
function nsPluginRequest(aPlugRequest){
  this.mimetype = encodeURI(aPlugRequest.mimetype);
  this.pluginsPage = aPlugRequest.pluginsPage;
}

function PluginInfo(aResult) {
  this.name = aResult.name;
  this.pid = aResult.pid;
  this.version = aResult.version;
  this.IconUrl = aResult.IconUrl;
  this.XPILocation = aResult.XPILocation;
  this.XPIHash = aResult.XPIHash;
  this.InstallerShowsUI = aResult.InstallerShowsUI;
  this.manualInstallationURL = aResult.manualInstallationURL;
  this.requestedMimetype = aResult.requestedMimetype;
  this.licenseURL = aResult.licenseURL;
  this.needsRestart = (aResult.needsRestart == "true");

  this.error = null;
  this.toBeInstalled = true;

  // no license provided, make it accepted
  this.licenseAccepted = this.licenseURL ? false : true;
}

var gPluginInstaller;

function wizardInit(){
  gPluginInstaller = new nsPluginInstallerWizard();
  gPluginInstaller.canAdvance(false);
  gPluginInstaller.getPluginData();
}

function wizardFinish(){
  // don't refresh if no plugins were found or installed
  if ((gPluginInstaller.mSuccessfullPluginInstallation > 0) &&
      (gPluginInstaller.mPluginInfoArray.length != 0) &&
      gPluginInstaller.mTab) {
    // clear the tab's plugin list only if we installed at least one plugin
    gPluginInstaller.mTab.missingPlugins = null;
    // reset UI
    window.opener.getBrowser().hideMessage(window.opener.getBrowser().selectedBrowser, "top");
    // reload the browser to make the new plugin show
    window.opener.getBrowser().reloadTab(gPluginInstaller.mTab);
  }

  return true;
}
