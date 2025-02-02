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

var PluginInstallService = {
  
  init: function () 
  {
  },

  pluginPidArray: null,

  startPluginInsallation: function (aPluginXPIUrlsArray,
                                    aPluginHashes,
                                    aPluginPidArray) {
     this.pluginPidArray = aPluginPidArray;

     var xpiManager = Components.classes["@mozilla.org/xpinstall/install-manager;1"]
                                .createInstance(Components.interfaces.nsIXPInstallManager);
     xpiManager.initManagerWithHashes(aPluginXPIUrlsArray, aPluginHashes,
                                      aPluginXPIUrlsArray.length, this);
  },

  // XPI progress listener stuff
  onStateChange: function (aIndex, aState, aValue)
  {
    // get the pid to return to the wizard
    var pid = this.pluginPidArray[aIndex];
    var errorMsg;

    if (aState == Components.interfaces.nsIXPIProgressDialog.INSTALL_DONE) {
      if (aValue != 0) {
        var xpinstallStrings = document.getElementById("xpinstallStrings");
        try {
          errorMsg = xpinstallStrings.getString("error" + aValue);
        }
        catch (e) {
          errorMsg = xpinstallStrings.getFormattedString("unknown.error", [aValue]);
        }
      }
    }

    gPluginInstaller.pluginInstallationProgress(pid, aState, errorMsg);

  },

  onProgress: function (aIndex, aValue, aMaxValue)
  {
    // get the pid to return to the wizard
    var pid = this.pluginPidArray[aIndex];

    gPluginInstaller.pluginInstallationProgressMeter(pid, aValue, aMaxValue);
  }
}
