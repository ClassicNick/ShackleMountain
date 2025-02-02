# -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
# 
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
# 
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
# 
# The Original Code is the Thunderbird Preferences System.
# 
# The Initial Developer of the Original Code is Scott MacGregor.
# Portions created by the Initial Developer are Copyright (C) 2005
# the Initial Developer. All Rights Reserved.
# 
# Contributor(s):
#   Scott MacGregor <mscott@mozilla.org>
# 
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
# 
# ***** END LICENSE BLOCK *****

var gAdvancedPane = {
  mPane: null,
  mInitialized: false,

  init: function ()
  {
    this.mPane = document.getElementById("paneAdvanced");
    this.updateMarkAsReadTextbox(false);

    if ("arguments" in window && window.arguments[1] && document.getElementById(window.arguments[1]))
      document.getElementById("advancedPrefs").selectedTab = document.getElementById(window.arguments[1]);
    else 
    {    
      var preference = document.getElementById("mail.preferences.advanced.selectedTabIndex");
      if (preference.value)
        document.getElementById("advancedPrefs").selectedIndex = preference.value;
    }

    this.updateAppUpdateItems();
    this.updateAutoItems();
    this.updateModeItems();   

    this.mInitialized = true;
  },

  tabSelectionChanged: function ()
  {
    if (this.mInitialized)
    {
      document.getElementById("mail.preferences.advanced.selectedTabIndex")
              .valueFromPreferences = document.getElementById("advancedPrefs").selectedIndex;
    }
  },

  showConfigEdit: function()
  {
    document.documentElement.openWindow("Preferences:ConfigManager",
                                        "chrome://global/content/config.xul",
                                        "", null);
  },

  updateButtons: function (aButtonID, aPreferenceID)
  {
    var button = document.getElementById(aButtonID);
    var preference = document.getElementById(aPreferenceID);
    // This is actually before the value changes, so the value is not as you expect. 
    button.disabled = preference.value == true;
    return undefined;
  },

  /**
   * UI state matrix for update preference conditions
   * 
   * UI Components:                                     Preferences
   * 1 = Thunderbird checkbox                               i   = app.update.enabled
   * 2 = When updates for Thunderbird are found label       ii  = app.update.auto
   * 3 = Automatic Radiogroup (Ask vs. Automatically)       iii = app.update.mode
   * 4 = Warn before disabling extensions checkbox
   * 
   * States:
   * Element     p   val     locked    Disabled 
   * 1           i   t/f     f         false 
   *             i   t/f     t         true
   *             ii  t/f     t/f       false
   *             iii 0/1/2   t/f       false
   * 2,3         i   t       t/f       false
   *             i   f       t/f       true
   *             ii  t/f     f         false
   *             ii  t/f     t         true
   *             iii 0/1/2   t/f       false
   * 4           i   t       t/f       false
   *             i   f       t/f       true
   *             ii  t       t/f       false
   *             ii  f       t/f       true
   *             iii 0/1/2   f         false
   *             iii 0/1/2   t         true   
   * 
   */
  updateAppUpdateItems: function () 
  {
    var aus = 
        Components.classes["@mozilla.org/updates/update-service;1"].
        getService(Components.interfaces.nsIApplicationUpdateService);

    var enabledPref = document.getElementById("app.update.enabled");
    var enableAppUpdate = document.getElementById("enableAppUpdate");

    enableAppUpdate.disabled = !aus.canUpdate || enabledPref.locked;
  },
  
  updateAutoItems: function () 
  {
    var enabledPref = document.getElementById("app.update.enabled");
    var autoPref = document.getElementById("app.update.auto");
    
    var updateModeLabel = document.getElementById("updateModeLabel");
    var updateMode = document.getElementById("updateMode");
    
    var disable = enabledPref.locked || !enabledPref.value ||
                  autoPref.locked;
    updateModeLabel.disabled = updateMode.disabled = disable;
  },

  updateModeItems: function () 
  {
    var enabledPref = document.getElementById("app.update.enabled");
    var autoPref = document.getElementById("app.update.auto");
    var modePref = document.getElementById("app.update.mode");
    
    var warnIncompatible = document.getElementById("warnIncompatible");
    
    var disable = enabledPref.locked || !enabledPref.value || autoPref.locked ||
                  !autoPref.value || modePref.locked;
    warnIncompatible.disabled = disable;
  },

  /**
   * The Extensions checkbox and button are disabled only if the enable Addon
   * update preference is locked. 
   */
  updateAddonUpdateUI: function ()
  {
    var enabledPref = document.getElementById("extensions.update.enabled");
    var enableAddonUpdate = document.getElementById("enableAddonUpdate");

    enableAddonUpdate.disabled = enabledPref.locked;
  },  

  /**
   * app.update.mode is a three state integer preference, and we have to 
   * express all three values in a single checkbox:
   * "Warn me if this will disable extensions or themes"
   * Preference Value         Checkbox State    Meaning
   * 0                        Unchecked         Do not warn
   * 1                        Checked           Warn if there are incompatibilities
   * 2                        Checked           Warn if there are incompatibilities,
   *                                            or the update is major.
   */
  _modePreference: -1,
  addonWarnSyncFrom: function ()
  {
    var preference = document.getElementById("app.update.mode");
    var doNotWarn = preference.value != 0;
    gAdvancedPane._modePreference = doNotWarn ? preference.value : 1;
    return doNotWarn;
  },
  
  addonWarnSyncTo: function ()
  {
    var warnIncompatible = document.getElementById("warnIncompatible");
    return !warnIncompatible.checked ? 0 : gAdvancedPane._modePreference;
  },
   
  showUpdates: function ()
  {
    var prompter = Components.classes["@mozilla.org/updates/update-prompt;1"]
                             .createInstance(Components.interfaces.nsIUpdatePrompt);
    prompter.showUpdateHistory(window);
  },

  updateMarkAsReadTextbox: function(aFocusTextBox) 
  {
    var textbox = document.getElementById('markAsReadDelay'); 
    textbox.disabled = !document.getElementById('markAsRead').checked;
    if (!textbox.disabled && aFocusTextBox)
        textbox.focus();
  }
};
