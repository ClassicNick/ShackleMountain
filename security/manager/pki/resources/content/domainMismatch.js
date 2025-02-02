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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Javier Delgadillo <javi@netscape.com>
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


const nsIPKIParamBlock    = Components.interfaces.nsIPKIParamBlock;
const nsIDialogParamBlock = Components.interfaces.nsIDialogParamBlock;
const nsIX509Cert         = Components.interfaces.nsIX509Cert;
const nsIX509Cert18Branch = Components.interfaces.nsIX509Cert18Branch;

var pkiParams;
var dialogParams;
var cert;

function onLoad()
{
  pkiParams = window.arguments[0].QueryInterface(nsIPKIParamBlock);  
  var isupport = pkiParams.getISupportAtIndex(1);
  cert     = isupport.QueryInterface(nsIX509Cert);
  dialogParams = pkiParams.QueryInterface(nsIDialogParamBlock);
  var connectURL = dialogParams.GetString(1); 

  var validNames;
  var cert18 = cert.QueryInterface(nsIX509Cert18Branch);
  if (cert18) {
    var tmp = new Object;
    var nameCount = cert18.getValidNames(tmp);
    if (nameCount > 1) {
      validNames = "(" + tmp.value + ")";
    }
    else {
      validNames = tmp.value;
    }
  }
  else {
    validNames = cert.commonName;
  }

  var bundle = srGetStrBundle("chrome://pippki/locale/pippki.properties");

  var message1 = bundle.formatStringFromName("mismatchDomainMsg1", 
                                             [ connectURL, validNames ],
                                             2);
  var message2 = bundle.formatStringFromName("mismatchDomainMsg2", 
                                             [ connectURL ],
                                              1);
  setText("message1", message1);
  setText("message2", message2);
  document.documentElement.getButton("accept").focus();
}

function viewCert()
{
  viewCertHelper(window, cert);
}

function doOK()
{
  dialogParams.SetInt(1,1);
  return true;
}

function doCancel()
{
  dialogParams.SetInt(1,0);
  return true;
}
