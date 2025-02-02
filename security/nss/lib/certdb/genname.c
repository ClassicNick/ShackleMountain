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
 * The Original Code is the Netscape security libraries.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1994-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#include "plarena.h"
#include "seccomon.h"
#include "secitem.h"
#include "secoidt.h"
#include "mcom_db.h"
#include "secasn1.h"
#include "secder.h"
#include "certt.h"
#include "cert.h"
#include "xconst.h"
#include "secerr.h"
#include "secoid.h"
#include "prprf.h"
#include "genname.h"



static const SEC_ASN1Template CERTNameConstraintTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(CERTNameConstraint) },
    { SEC_ASN1_ANY, offsetof(CERTNameConstraint, DERName) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONTEXT_SPECIFIC | 0, 
          offsetof(CERTNameConstraint, min), SEC_IntegerTemplate }, 
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONTEXT_SPECIFIC | 1, 
          offsetof(CERTNameConstraint, max), SEC_IntegerTemplate },
    { 0, }
};

const SEC_ASN1Template CERT_NameConstraintSubtreeSubTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_AnyTemplate }
};


const SEC_ASN1Template CERT_NameConstraintSubtreePermitedTemplate[] = {
    { SEC_ASN1_CONTEXT_SPECIFIC | 0, 0, CERT_NameConstraintSubtreeSubTemplate }
};

const SEC_ASN1Template CERT_NameConstraintSubtreeExcludedTemplate[] = {
    { SEC_ASN1_CONTEXT_SPECIFIC | 1, 0, CERT_NameConstraintSubtreeSubTemplate }
};


static const SEC_ASN1Template CERTNameConstraintsTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(CERTNameConstraints) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONTEXT_SPECIFIC | 0, 
          offsetof(CERTNameConstraints, DERPermited), 
	  CERT_NameConstraintSubtreeSubTemplate},
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONTEXT_SPECIFIC | 1, 
          offsetof(CERTNameConstraints, DERExcluded), 
	  CERT_NameConstraintSubtreeSubTemplate},
    { 0, }
};


static const SEC_ASN1Template CERTOthNameTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(OtherName) },
    { SEC_ASN1_OBJECT_ID, 
	  offsetof(OtherName, oid) },
    { SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_CONSTRUCTED | SEC_ASN1_EXPLICIT | 0,
          offsetof(OtherName, name), SEC_AnyTemplate },
    { 0, } 
};

static const SEC_ASN1Template CERTOtherNameTemplate[] = {
    { SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_CONSTRUCTED | 0 ,
      offsetof(CERTGeneralName, name.OthName), CERTOthNameTemplate, 
      sizeof(CERTGeneralName) }
};

static const SEC_ASN1Template CERTOtherName2Template[] = {
    { SEC_ASN1_SEQUENCE | SEC_ASN1_CONTEXT_SPECIFIC | 0 ,
      0, NULL, sizeof(CERTGeneralName) },
    { SEC_ASN1_OBJECT_ID,
	  offsetof(CERTGeneralName, name.OthName) + offsetof(OtherName, oid) },
    { SEC_ASN1_ANY,
	  offsetof(CERTGeneralName, name.OthName) + offsetof(OtherName, name) },
    { 0, } 
};

static const SEC_ASN1Template CERT_RFC822NameTemplate[] = {
    { SEC_ASN1_CONTEXT_SPECIFIC | 1 ,
          offsetof(CERTGeneralName, name.other), SEC_IA5StringTemplate,
          sizeof (CERTGeneralName)}
};

static const SEC_ASN1Template CERT_DNSNameTemplate[] = {
    { SEC_ASN1_CONTEXT_SPECIFIC | 2 ,
          offsetof(CERTGeneralName, name.other), SEC_IA5StringTemplate,
          sizeof (CERTGeneralName)}
};

static const SEC_ASN1Template CERT_X400AddressTemplate[] = {
    { SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_CONSTRUCTED | 3,
          offsetof(CERTGeneralName, name.other), SEC_AnyTemplate,
          sizeof (CERTGeneralName)}
};

static const SEC_ASN1Template CERT_DirectoryNameTemplate[] = {
    { SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_CONSTRUCTED | SEC_ASN1_EXPLICIT | 4,
          offsetof(CERTGeneralName, derDirectoryName), SEC_AnyTemplate,
          sizeof (CERTGeneralName)}
};


static const SEC_ASN1Template CERT_EDIPartyNameTemplate[] = {
    { SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_CONSTRUCTED | 5,
          offsetof(CERTGeneralName, name.other), SEC_AnyTemplate,
          sizeof (CERTGeneralName)}
};

static const SEC_ASN1Template CERT_URITemplate[] = {
    { SEC_ASN1_CONTEXT_SPECIFIC | 6 ,
          offsetof(CERTGeneralName, name.other), SEC_IA5StringTemplate,
          sizeof (CERTGeneralName)}
};

static const SEC_ASN1Template CERT_IPAddressTemplate[] = {
    { SEC_ASN1_CONTEXT_SPECIFIC | 7 ,
          offsetof(CERTGeneralName, name.other), SEC_OctetStringTemplate,
          sizeof (CERTGeneralName)}
};

static const SEC_ASN1Template CERT_RegisteredIDTemplate[] = {
    { SEC_ASN1_CONTEXT_SPECIFIC | 8 ,
          offsetof(CERTGeneralName, name.other), SEC_ObjectIDTemplate,
          sizeof (CERTGeneralName)}
};


const SEC_ASN1Template CERT_GeneralNamesTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_AnyTemplate }
};



CERTGeneralName *
cert_NewGeneralName(PLArenaPool *arena, CERTGeneralNameType type)
{
    CERTGeneralName *name = arena 
                            ? PORT_ArenaZNew(arena, CERTGeneralName)
	                    : PORT_ZNew(CERTGeneralName);
    if (name) {
	name->type = type;
	name->l.prev = name->l.next = &name->l;
    }
    return name;
}

/* Copy content of one General Name to another.
** Caller has allocated destination general name.
** This function does not change the destinate's GeneralName's list linkage.
*/
SECStatus
cert_CopyOneGeneralName(PRArenaPool      *arena, 
		        CERTGeneralName  *dest, 
		        CERTGeneralName  *src)
{
    SECStatus rv;

    /* TODO: mark arena */
    PORT_Assert(dest != NULL);
    dest->type = src->type;

    switch (src->type) {
    case certDirectoryName: 
	rv = SECITEM_CopyItem(arena, &dest->derDirectoryName, 
				      &src->derDirectoryName);
	if (rv == SECSuccess) 
	    rv = CERT_CopyName(arena, &dest->name.directoryName, 
				       &src->name.directoryName);
	break;

    case certOtherName: 
	rv = SECITEM_CopyItem(arena, &dest->name.OthName.name, 
				      &src->name.OthName.name);
	if (rv == SECSuccess) 
	    rv = SECITEM_CopyItem(arena, &dest->name.OthName.oid, 
					  &src->name.OthName.oid);
	break;

    default: 
	rv = SECITEM_CopyItem(arena, &dest->name.other, 
				      &src->name.other);
	break;

    }
    if (rv != SECSuccess) {
	/* TODO: release back to mark */
    } else {
	/* TODO: unmark arena */
    }
    return rv;
}


void
CERT_DestroyGeneralNameList(CERTGeneralNameList *list)
{
    PZLock *lock;

    if (list != NULL) {
	lock = list->lock;
	PZ_Lock(lock);
	if (--list->refCount <= 0 && list->arena != NULL) {
	    PORT_FreeArena(list->arena, PR_FALSE);
	    PZ_Unlock(lock);
	    PZ_DestroyLock(lock);
	} else {
	    PZ_Unlock(lock);
	}
    }
    return;
}

CERTGeneralNameList *
CERT_CreateGeneralNameList(CERTGeneralName *name) {
    PRArenaPool *arena;
    CERTGeneralNameList *list = NULL;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	goto done;
    }
    list = PORT_ArenaZNew(arena, CERTGeneralNameList);
    if (!list)
    	goto loser;
    if (name != NULL) {
	SECStatus rv;
	list->name = cert_NewGeneralName(arena, (CERTGeneralNameType)0);
	if (!list->name)
	    goto loser;
	rv = CERT_CopyGeneralName(arena, list->name, name);
	if (rv != SECSuccess)
	    goto loser;
    }
    list->lock = PZ_NewLock(nssILockList);
    if (!list->lock)
    	goto loser;
    list->arena = arena;
    list->refCount = 1;
done:
    return list;

loser:
    PORT_FreeArena(arena, PR_FALSE);
    return NULL;
}

CERTGeneralName *
CERT_GetNextGeneralName(CERTGeneralName *current)
{
    PRCList *next;
    
    next = current->l.next;
    return (CERTGeneralName *) (((char *) next) - offsetof(CERTGeneralName, l));
}

CERTGeneralName *
CERT_GetPrevGeneralName(CERTGeneralName *current)
{
    PRCList *prev;
    prev = current->l.prev;
    return (CERTGeneralName *) (((char *) prev) - offsetof(CERTGeneralName, l));
}

CERTNameConstraint *
CERT_GetNextNameConstraint(CERTNameConstraint *current)
{
    PRCList *next;
    
    next = current->l.next;
    return (CERTNameConstraint *) (((char *) next) - offsetof(CERTNameConstraint, l));
}

CERTNameConstraint *
CERT_GetPrevNameConstraint(CERTNameConstraint *current)
{
    PRCList *prev;
    prev = current->l.prev;
    return (CERTNameConstraint *) (((char *) prev) - offsetof(CERTNameConstraint, l));
}

SECItem *
CERT_EncodeGeneralName(CERTGeneralName *genName, SECItem *dest, PRArenaPool *arena)
{

    const SEC_ASN1Template * template;

    PORT_Assert(arena);
    if (arena == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return NULL;
    }
    /* TODO: mark arena */
    if (dest == NULL) {
	dest = PORT_ArenaZNew(arena, SECItem);
	if (!dest)
	    goto loser;
    }
    if (genName->type == certDirectoryName) {
	if (genName->derDirectoryName.data == NULL) {
	    /* The field hasn't been encoded yet. */
            SECItem * pre_dest =
            SEC_ASN1EncodeItem (arena, &(genName->derDirectoryName),
                                &(genName->name.directoryName),
                                CERT_NameTemplate);
            if (!pre_dest)
                goto loser;
	}
	if (genName->derDirectoryName.data == NULL) {
	    goto loser;
	}
    }
    switch (genName->type) {
    case certURI:           template = CERT_URITemplate;           break;
    case certRFC822Name:    template = CERT_RFC822NameTemplate;    break;
    case certDNSName:       template = CERT_DNSNameTemplate;       break;
    case certIPAddress:     template = CERT_IPAddressTemplate;     break;
    case certOtherName:     template = CERTOtherNameTemplate;      break;
    case certRegisterID:    template = CERT_RegisteredIDTemplate;  break;
         /* for this type, we expect the value is already encoded */
    case certEDIPartyName:  template = CERT_EDIPartyNameTemplate;  break;
	 /* for this type, we expect the value is already encoded */
    case certX400Address:   template = CERT_X400AddressTemplate;   break;
    case certDirectoryName: template = CERT_DirectoryNameTemplate; break;
    default:
	PORT_Assert(0); goto loser;
    }
    dest = SEC_ASN1EncodeItem(arena, dest, genName, template);
    if (!dest) {
	goto loser;
    }
    /* TODO: unmark arena */
    return dest;
loser:
    /* TODO: release arena back to mark */
    return NULL;
}

SECItem **
cert_EncodeGeneralNames(PRArenaPool *arena, CERTGeneralName *names)
{
    CERTGeneralName  *current_name;
    SECItem          **items = NULL;
    int              count = 0;
    int              i;
    PRCList          *head;

    PORT_Assert(arena);
    /* TODO: mark arena */
    current_name = names;
    if (names != NULL) {
	count = 1;
    }
    head = &(names->l);
    while (current_name->l.next != head) {
	current_name = CERT_GetNextGeneralName(current_name);
	++count;
    }
    current_name = CERT_GetNextGeneralName(current_name);
    items = PORT_ArenaNewArray(arena, SECItem *, count + 1);
    if (items == NULL) {
	goto loser;
    }
    for (i = 0; i < count; i++) {
	items[i] = CERT_EncodeGeneralName(current_name, (SECItem *)NULL, arena);
	if (items[i] == NULL) {
	    goto loser;
	}
	current_name = CERT_GetNextGeneralName(current_name);
    }
    items[i] = NULL;
    /* TODO: unmark arena */
    return items;
loser:
    /* TODO: release arena to mark */
    return NULL;
}

CERTGeneralName *
CERT_DecodeGeneralName(PRArenaPool      *arena,
		       SECItem          *encodedName,
		       CERTGeneralName  *genName)
{
    const SEC_ASN1Template *         template;
    CERTGeneralNameType              genNameType;
    SECStatus                        rv = SECSuccess;

    PORT_Assert(arena);
    /* TODO: mark arena */
    genNameType = (CERTGeneralNameType)((*(encodedName->data) & 0x0f) + 1);
    if (genName == NULL) {
	genName = cert_NewGeneralName(arena, genNameType);
	if (!genName)
	    goto loser;
    } else {
	genName->type = genNameType;
	genName->l.prev = genName->l.next = &genName->l;
    }
    switch (genNameType) {
    case certURI: 		template = CERT_URITemplate;           break;
    case certRFC822Name: 	template = CERT_RFC822NameTemplate;    break;
    case certDNSName: 		template = CERT_DNSNameTemplate;       break;
    case certIPAddress: 	template = CERT_IPAddressTemplate;     break;
    case certOtherName: 	template = CERTOtherNameTemplate;      break;
    case certRegisterID: 	template = CERT_RegisteredIDTemplate;  break;
    case certEDIPartyName: 	template = CERT_EDIPartyNameTemplate;  break;
    case certX400Address: 	template = CERT_X400AddressTemplate;   break;
    case certDirectoryName: 	template = CERT_DirectoryNameTemplate; break;
    default: 
        goto loser;
    }
    rv = SEC_ASN1DecodeItem(arena, genName, template, encodedName);
    if (rv != SECSuccess) 
	goto loser;
    if (genNameType == certDirectoryName) {
	rv = SEC_ASN1DecodeItem(arena, &(genName->name.directoryName), 
				CERT_NameTemplate, 
				&(genName->derDirectoryName));
        if (rv != SECSuccess)
	    goto loser;
    }

    /* TODO: unmark arena */
    return genName;
loser:
    /* TODO: release arena to mark */
    return NULL;
}

CERTGeneralName *
cert_DecodeGeneralNames (PRArenaPool  *arena,
			 SECItem      **encodedGenName)
{
    PRCList                           *head = NULL;
    PRCList                           *tail = NULL;
    CERTGeneralName                   *currentName = NULL;

    PORT_Assert(arena);
    if (!encodedGenName || !arena) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return NULL;
    }
    /* TODO: mark arena */
    while (*encodedGenName != NULL) {
	currentName = CERT_DecodeGeneralName(arena, *encodedGenName, NULL);
	if (currentName == NULL)
	    break;
	if (head == NULL) {
	    head = &(currentName->l);
	    tail = head;
	}
	currentName->l.next = head;
	currentName->l.prev = tail;
	tail = head->prev = tail->next = &(currentName->l);
	encodedGenName++;
    }
    if (currentName) {
	/* TODO: unmark arena */
	return CERT_GetNextGeneralName(currentName);
    }
    /* TODO: release arena to mark */
    return NULL;
}

void
CERT_DestroyGeneralName(CERTGeneralName *name)
{
    cert_DestroyGeneralNames(name);
}

SECStatus
cert_DestroyGeneralNames(CERTGeneralName *name)
{
    CERTGeneralName    *first;
    CERTGeneralName    *next = NULL;


    first = name;
    do {
	next = CERT_GetNextGeneralName(name);
	PORT_Free(name);
	name = next;
    } while (name != first);
    return SECSuccess;
}

static SECItem *
cert_EncodeNameConstraint(CERTNameConstraint  *constraint, 
			 SECItem             *dest,
			 PRArenaPool         *arena)
{
    PORT_Assert(arena);
    if (dest == NULL) {
	dest = PORT_ArenaZNew(arena, SECItem);
	if (dest == NULL) {
	    return NULL;
	}
    }
    CERT_EncodeGeneralName(&(constraint->name), &(constraint->DERName), arena);
    
    dest = SEC_ASN1EncodeItem (arena, dest, constraint,
			       CERTNameConstraintTemplate);
    return dest;
} 

SECStatus 
cert_EncodeNameConstraintSubTree(CERTNameConstraint  *constraints,
			         PRArenaPool         *arena,
				 SECItem             ***dest,
				 PRBool              permited)
{
    CERTNameConstraint  *current_constraint = constraints;
    SECItem             **items = NULL;
    int                 count = 0;
    int                 i;
    PRCList             *head;

    PORT_Assert(arena);
    /* TODO: mark arena */
    if (constraints != NULL) {
	count = 1;
    }
    head = &constraints->l;
    while (current_constraint->l.next != head) {
	current_constraint = CERT_GetNextNameConstraint(current_constraint);
	++count;
    }
    current_constraint = CERT_GetNextNameConstraint(current_constraint);
    items = PORT_ArenaZNewArray(arena, SECItem *, count + 1);
    if (items == NULL) {
	goto loser;
    }
    for (i = 0; i < count; i++) {
	items[i] = cert_EncodeNameConstraint(current_constraint, 
					     (SECItem *) NULL, arena);
	if (items[i] == NULL) {
	    goto loser;
	}
	current_constraint = CERT_GetNextNameConstraint(current_constraint);
    }
    *dest = items;
    if (*dest == NULL) {
	goto loser;
    }
    /* TODO: unmark arena */
    return SECSuccess;
loser:
    /* TODO: release arena to mark */
    return SECFailure;
}

SECStatus 
cert_EncodeNameConstraints(CERTNameConstraints  *constraints,
			   PRArenaPool          *arena,
			   SECItem              *dest)
{
    SECStatus    rv = SECSuccess;

    PORT_Assert(arena);
    /* TODO: mark arena */
    if (constraints->permited != NULL) {
	rv = cert_EncodeNameConstraintSubTree(constraints->permited, arena,
					      &constraints->DERPermited, 
					      PR_TRUE);
	if (rv == SECFailure) {
	    goto loser;
	}
    }
    if (constraints->excluded != NULL) {
	rv = cert_EncodeNameConstraintSubTree(constraints->excluded, arena,
					      &constraints->DERExcluded, 
					      PR_FALSE);
	if (rv == SECFailure) {
	    goto loser;
	}
    }
    dest = SEC_ASN1EncodeItem(arena, dest, constraints, 
			      CERTNameConstraintsTemplate);
    if (dest == NULL) {
	goto loser;
    }
    /* TODO: unmark arena */
    return SECSuccess;
loser:
    /* TODO: release arena to mark */
    return SECFailure;
}


CERTNameConstraint *
cert_DecodeNameConstraint(PRArenaPool       *arena,
			  SECItem           *encodedConstraint)
{
    CERTNameConstraint     *constraint;
    SECStatus              rv = SECSuccess;
    CERTGeneralName        *temp;

    PORT_Assert(arena);
    /* TODO: mark arena */
    constraint = PORT_ArenaZNew(arena, CERTNameConstraint);
    if (!constraint)
    	goto loser;
    rv = SEC_ASN1DecodeItem(arena, constraint, CERTNameConstraintTemplate, 
                            encodedConstraint);
    if (rv != SECSuccess) {
	goto loser;
    }
    temp = CERT_DecodeGeneralName(arena, &(constraint->DERName), 
                                         &(constraint->name));
    if (temp != &(constraint->name)) {
	goto loser;
    }

    /* ### sjlee: since the name constraint contains only one 
     *            CERTGeneralName, the list within CERTGeneralName shouldn't 
     *            point anywhere else.  Otherwise, bad things will happen.
     */
    constraint->name.l.prev = constraint->name.l.next = &(constraint->name.l);
    /* TODO: unmark arena */
    return constraint;
loser:
    /* TODO: release arena back to mark */
    return NULL;
}

CERTNameConstraint *
cert_DecodeNameConstraintSubTree(PRArenaPool   *arena,
				 SECItem       **subTree,
				 PRBool        permited)
{
    CERTNameConstraint   *current = NULL;
    CERTNameConstraint   *first = NULL;
    CERTNameConstraint   *last = NULL;
    int                  i = 0;

    PORT_Assert(arena);
    /* TODO: mark arena */
    while (subTree[i] != NULL) {
	current = cert_DecodeNameConstraint(arena, subTree[i]);
	if (current == NULL) {
	    goto loser;
	}
	if (last == NULL) {
	    first = last = current;
	}
	current->l.prev = &(last->l);
	current->l.next = last->l.next;
	last->l.next = &(current->l);
	i++;
    }
    first->l.prev = &(current->l);
    /* TODO: unmark arena */
    return first;
loser:
    /* TODO: release arena back to mark */
    return NULL;
}

CERTNameConstraints *
cert_DecodeNameConstraints(PRArenaPool   *arena,
			   SECItem       *encodedConstraints)
{
    CERTNameConstraints   *constraints;
    SECStatus             rv;

    PORT_Assert(arena);
    PORT_Assert(encodedConstraints);
    /* TODO: mark arena */
    constraints = PORT_ArenaZNew(arena, CERTNameConstraints);
    if (constraints == NULL) {
	goto loser;
    }
    rv = SEC_ASN1DecodeItem(arena, constraints, CERTNameConstraintsTemplate, 
			    encodedConstraints);
    if (rv != SECSuccess) {
	goto loser;
    }
    if (constraints->DERPermited != NULL && 
        constraints->DERPermited[0] != NULL) {
	constraints->permited = 
	    cert_DecodeNameConstraintSubTree(arena, constraints->DERPermited,
					     PR_TRUE);
	if (constraints->permited == NULL) {
	    goto loser;
	}
    }
    if (constraints->DERExcluded != NULL && 
        constraints->DERExcluded[0] != NULL) {
	constraints->excluded = 
	    cert_DecodeNameConstraintSubTree(arena, constraints->DERExcluded,
					     PR_FALSE);
	if (constraints->excluded == NULL) {
	    goto loser;
	}
    }
    /* TODO: unmark arena */
    return constraints;
loser:
    /* TODO: release arena back to mark */
    return NULL;
}

/* Copy a chain of one or more general names to a destination chain.
** Caller has allocated at least the first destination GeneralName struct. 
** Both source and destination chains are circular doubly-linked lists.
** The first source struct is copied to the first destination struct.
** If the source chain has more than one member, and the destination chain 
** has only one member, then this function allocates new structs for all but 
** the first copy from the arena and links them into the destination list.  
** If the destination struct is part of a list with more than one member,
** then this function traverses both the source and destination lists,
** copying each source struct to the corresponding dest struct.
** In that case, the destination list MUST contain at least as many 
** structs as the source list or some dest entries will be overwritten.
*/
SECStatus
CERT_CopyGeneralName(PRArenaPool      *arena, 
		     CERTGeneralName  *dest, 
		     CERTGeneralName  *src)
{
    SECStatus rv;
    CERTGeneralName *destHead = dest;
    CERTGeneralName *srcHead = src;

    PORT_Assert(dest != NULL);
    if (!dest) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    /* TODO: mark arena */
    do {
	rv = cert_CopyOneGeneralName(arena, dest, src);
	if (rv != SECSuccess)
	    goto loser;
	src = CERT_GetNextGeneralName(src);
	/* if there is only one general name, we shouldn't do this */
	if (src != srcHead) {
	    if (dest->l.next == &destHead->l) {
		CERTGeneralName *temp;
		temp = cert_NewGeneralName(arena, (CERTGeneralNameType)0);
		if (!temp) 
		    goto loser;
		temp->l.next = &destHead->l;
		temp->l.prev = &dest->l;
		destHead->l.prev = &temp->l;
		dest->l.next = &temp->l;
		dest = temp;
	    } else {
		dest = CERT_GetNextGeneralName(dest);
	    }
	}
    } while (src != srcHead && rv == SECSuccess);
    /* TODO: unmark arena */
    return rv;
loser:
    /* TODO: release back to mark */
    return SECFailure;
}


CERTGeneralNameList *
CERT_DupGeneralNameList(CERTGeneralNameList *list)
{
    if (list != NULL) {
	PZ_Lock(list->lock);
	list->refCount++;
	PZ_Unlock(list->lock);
    }
    return list;
}

CERTNameConstraint *
CERT_CopyNameConstraint(PRArenaPool         *arena, 
			CERTNameConstraint  *dest, 
			CERTNameConstraint  *src)
{
    SECStatus  rv;
    
    /* TODO: mark arena */
    if (dest == NULL) {
	dest = PORT_ArenaZNew(arena, CERTNameConstraint);
	if (!dest)
	    goto loser;
	/* mark that it is not linked */
	dest->name.l.prev = dest->name.l.next = &(dest->name.l);
    }
    rv = CERT_CopyGeneralName(arena, &dest->name, &src->name);
    if (rv != SECSuccess) {
	goto loser;
    }
    rv = SECITEM_CopyItem(arena, &dest->DERName, &src->DERName);
    if (rv != SECSuccess) {
	goto loser;
    }
    rv = SECITEM_CopyItem(arena, &dest->min, &src->min);
    if (rv != SECSuccess) {
	goto loser;
    }
    rv = SECITEM_CopyItem(arena, &dest->max, &src->max);
    if (rv != SECSuccess) {
	goto loser;
    }
    dest->l.prev = dest->l.next = &dest->l;
    /* TODO: unmark arena */
    return dest;
loser:
    /* TODO: release arena to mark */
    return NULL;
}


CERTGeneralName *
cert_CombineNamesLists(CERTGeneralName *list1, CERTGeneralName *list2)
{
    PRCList *begin1;
    PRCList *begin2;
    PRCList *end1;
    PRCList *end2;

    if (list1 == NULL){
	return list2;
    } else if (list2 == NULL) {
	return list1;
    } else {
	begin1 = &list1->l;
	begin2 = &list2->l;
	end1 = list1->l.prev;
	end2 = list2->l.prev;
	end1->next = begin2;
	end2->next = begin1;
	begin1->prev = end2;
	begin2->prev = end1;
	return list1;
    }
}


CERTNameConstraint *
cert_CombineConstraintsLists(CERTNameConstraint *list1, CERTNameConstraint *list2)
{
    PRCList *begin1;
    PRCList *begin2;
    PRCList *end1;
    PRCList *end2;

    if (list1 == NULL){
	return list2;
    } else if (list2 == NULL) {
	return list1;
    } else {
	begin1 = &list1->l;
	begin2 = &list2->l;
	end1 = list1->l.prev;
	end2 = list2->l.prev;
	end1->next = begin2;
	end2->next = begin1;
	begin1->prev = end2;
	begin2->prev = end1;
	return list1;
    }
}


CERTNameConstraint *
CERT_AddNameConstraint(CERTNameConstraint *list, 
		       CERTNameConstraint *constraint)
{
    PORT_Assert(constraint != NULL);
    constraint->l.next = constraint->l.prev = &constraint->l;
    list = cert_CombineConstraintsLists(list, constraint);
    return list;
}


SECStatus
CERT_GetNameConstraintByType (CERTNameConstraint *constraints,
			      CERTGeneralNameType type, 
			      CERTNameConstraint **returnList,
			      PRArenaPool *arena)
{
    CERTNameConstraint *current;
    
    *returnList = NULL;
    if (!constraints)
	return SECSuccess;
    /* TODO: mark arena */
    current = constraints;
    do {
	PORT_Assert(current->name.type);
	if (current->name.type == type) {
	    CERTNameConstraint *temp;
	    temp = CERT_CopyNameConstraint(arena, NULL, current);
	    if (temp == NULL) 
		goto loser;
	    *returnList = CERT_AddNameConstraint(*returnList, temp);
	}
	current = CERT_GetNextNameConstraint(current);
    } while (current != constraints);
    /* TODO: unmark arena */
    return SECSuccess;
loser:
    /* TODO: release arena back to mark */
    return SECFailure;
}

void *
CERT_GetGeneralNameByType (CERTGeneralName *genNames,
			   CERTGeneralNameType type, PRBool derFormat)
{
    CERTGeneralName *current;
    
    if (!genNames)
	return NULL;
    current = genNames;

    do {
	if (current->type == type) {
	    switch (type) {
	    case certDNSName:
	    case certEDIPartyName:
	    case certIPAddress:
	    case certRegisterID:
	    case certRFC822Name:
	    case certX400Address:
	    case certURI: 
		return (void *)&current->name.other;           /* SECItem * */

	    case certOtherName: 
		return (void *)&current->name.OthName;         /* OthName * */

	    case certDirectoryName: 
		return derFormat 
		       ? (void *)&current->derDirectoryName    /* SECItem * */
		       : (void *)&current->name.directoryName; /* CERTName * */
	    }
	    PORT_Assert(0); 
	    return NULL;
	}
	current = CERT_GetNextGeneralName(current);
    } while (current != genNames);
    return NULL;
}

int
CERT_GetNamesLength(CERTGeneralName *names)
{
    int              length = 0;
    CERTGeneralName  *first;

    first = names;
    if (names != NULL) {
	do {
	    length++;
	    names = CERT_GetNextGeneralName(names);
	} while (names != first);
    }
    return length;
}

/* Creates new GeneralNames for any email addresses found in the 
** input DN, and links them onto the list for the DN.
*/
SECStatus
cert_ExtractDNEmailAddrs(CERTGeneralName *name, PLArenaPool *arena)
{
    CERTGeneralName  *nameList = NULL;
    const CERTRDN    **nRDNs   = name->name.directoryName.rdns;
    SECStatus        rv        = SECSuccess;

    PORT_Assert(name->type == certDirectoryName);
    if (name->type != certDirectoryName) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    /* TODO: mark arena */
    while (nRDNs && *nRDNs) { /* loop over RDNs */
	const CERTRDN *nRDN = *nRDNs++;
	CERTAVA **nAVAs = nRDN->avas;
	while (nAVAs && *nAVAs) { /* loop over AVAs */
	    int tag;
	    CERTAVA *nAVA = *nAVAs++;
	    tag = CERT_GetAVATag(nAVA);
	    if ( tag == SEC_OID_PKCS9_EMAIL_ADDRESS ||
		 tag == SEC_OID_RFC1274_MAIL) { /* email AVA */
		CERTGeneralName *newName = NULL;
		SECItem *avaValue = CERT_DecodeAVAValue(&nAVA->value);
		if (!avaValue)
		    goto loser;
		rv = SECFailure;
                newName = cert_NewGeneralName(arena, certRFC822Name);
		if (newName) {
		   rv = SECITEM_CopyItem(arena, &newName->name.other, avaValue);
		}
		SECITEM_FreeItem(avaValue, PR_TRUE);
		if (rv != SECSuccess)
		    goto loser;
		nameList = cert_CombineNamesLists(nameList, newName);
	    } /* handle one email AVA */
	} /* loop over AVAs */
    } /* loop over RDNs */
    /* combine new names with old one. */
    name = cert_CombineNamesLists(name, nameList);
    /* TODO: unmark arena */
    return SECSuccess;

loser:
    /* TODO: release arena back to mark */
    return SECFailure;
}

/* This function is called by CERT_VerifyCertChain to extract all
** names from a cert in preparation for a name constraints test.
*/
CERTGeneralName *
CERT_GetCertificateNames(CERTCertificate *cert, PRArenaPool *arena)
{
    CERTGeneralName  *DN;
    CERTGeneralName  *altName         = NULL;
    SECItem          altNameExtension = {siBuffer, NULL, 0 };
    SECStatus        rv;

    /* TODO: mark arena */
    DN = cert_NewGeneralName(arena, certDirectoryName);
    if (DN == NULL) {
	goto loser;
    }
    rv = CERT_CopyName(arena, &DN->name.directoryName, &cert->subject);
    if (rv != SECSuccess) {
	goto loser;
    }
    rv = SECITEM_CopyItem(arena, &DN->derDirectoryName, &cert->derSubject);
    if (rv != SECSuccess) {
	goto loser;
    }
    /* Extract email addresses from DN, construct CERTGeneralName structs 
    ** for them, add them to the name list 
    */
    rv = cert_ExtractDNEmailAddrs(DN, arena);
    if (rv != SECSuccess)
        goto loser;

    /* Now extract any GeneralNames from the subject name names extension. */
    rv = CERT_FindCertExtension(cert, SEC_OID_X509_SUBJECT_ALT_NAME, 
				&altNameExtension);
    if (rv == SECSuccess) {
	altName = CERT_DecodeAltNameExtension(arena, &altNameExtension);
	rv = altName ? SECSuccess : SECFailure;
    }
    if (rv != SECSuccess && PORT_GetError() == SEC_ERROR_EXTENSION_NOT_FOUND)
	rv = SECSuccess;
    if (altNameExtension.data)
	SECITEM_FreeItem(&altNameExtension, PR_FALSE);
    if (rv != SECSuccess)
        goto loser;
    DN = cert_CombineNamesLists(DN, altName);

    /* TODO: unmark arena */
    return DN;
loser:
    /* TODO: release arena to mark */
    return NULL;
}

/* Returns SECSuccess if name matches constraint per RFC 3280 rules for 
** URI name constraints.  SECFailure otherwise.
** If the constraint begins with a dot, it is a domain name, otherwise
** It is a host name.  Examples:
**  Constraint            Name             Result
** ------------      ---------------      --------
**  foo.bar.com          foo.bar.com      matches
**  foo.bar.com          FoO.bAr.CoM      matches
**  foo.bar.com      www.foo.bar.com      no match
**  foo.bar.com        nofoo.bar.com      no match
** .foo.bar.com      www.foo.bar.com      matches
** .foo.bar.com        nofoo.bar.com      no match
** .foo.bar.com          foo.bar.com      no match
** .foo.bar.com     www..foo.bar.com      no match
*/
static SECStatus
compareURIN2C(const SECItem *name, const SECItem *constraint)
{
    int offset;
    /* The spec is silent on intepreting zero-length constraints.
    ** We interpret them as matching no URI names.
    */
    if (!constraint->len)
        return SECFailure;
    if (constraint->data[0] != '.') { 
    	/* constraint is a host name. */
    	if (name->len != constraint->len ||
	    PL_strncasecmp(name->data, constraint->data, constraint->len))
	    return SECFailure;
    	return SECSuccess;
    }
    /* constraint is a domain name. */
    if (name->len < constraint->len)
        return SECFailure;
    offset = name->len - constraint->len;
    if (PL_strncasecmp(name->data + offset, constraint->data, constraint->len))
        return SECFailure;
    if (!offset || 
        (name->data[offset - 1] == '.') + (constraint->data[0] == '.') == 1)
	return SECSuccess;
    return SECFailure;
}

/* for DNSname constraints, RFC 3280 says, (section 4.2.1.11, page 38)
**
** DNS name restrictions are expressed as foo.bar.com.  Any DNS name
** that can be constructed by simply adding to the left hand side of the
** name satisfies the name constraint.  For example, www.foo.bar.com
** would satisfy the constraint but foo1.bar.com would not.
**
** But NIST's PKITS test suite requires that the constraint be treated
** as a domain name, and requires that any name added to the left hand
** side end in a dot ".".  Sensible, but not strictly following the RFC.
**
**  Constraint            Name            RFC 3280  NIST PKITS
** ------------      ---------------      --------  ----------
**  foo.bar.com          foo.bar.com      matches    matches
**  foo.bar.com          FoO.bAr.CoM      matches    matches
**  foo.bar.com      www.foo.bar.com      matches    matches
**  foo.bar.com        nofoo.bar.com      MATCHES    NO MATCH
** .foo.bar.com      www.foo.bar.com      matches    matches? disallowed?
** .foo.bar.com          foo.bar.com      no match   no match
** .foo.bar.com     www..foo.bar.com      matches    probably not 
**
** We will try to conform to NIST's PKITS tests, and the unstated 
** rules they imply.
*/
static SECStatus
compareDNSN2C(const SECItem *name, const SECItem *constraint)
{
    int offset;
    /* The spec is silent on intepreting zero-length constraints.
    ** We interpret them as matching all DNSnames.
    */
    if (!constraint->len)
        return SECSuccess;
    if (name->len < constraint->len)
        return SECFailure;
    offset = name->len - constraint->len;
    if (PL_strncasecmp(name->data + offset, constraint->data, constraint->len))
        return SECFailure;
    if (!offset || 
        (name->data[offset - 1] == '.') + (constraint->data[0] == '.') == 1)
	return SECSuccess;
    return SECFailure;
}

/* Returns SECSuccess if name matches constraint per RFC 3280 rules for
** internet email addresses.  SECFailure otherwise.
** If constraint contains a '@' then the two strings much match exactly.
** Else if constraint starts with a '.'. then it must match the right-most
** substring of the name, 
** else constraint string must match entire name after the name's '@'.
** Empty constraint string matches all names. All comparisons case insensitive.
*/
static SECStatus
compareRFC822N2C(const SECItem *name, const SECItem *constraint)
{
    int offset;
    if (!constraint->len)
        return SECSuccess;
    if (name->len < constraint->len)
        return SECFailure;
    if (constraint->len == 1 && constraint->data[0] == '.')
        return SECSuccess;
    for (offset = constraint->len - 1; offset >= 0; --offset) {
    	if (constraint->data[offset] == '@') {
	    return (name->len == constraint->len && 
	        !PL_strncasecmp(name->data, constraint->data, constraint->len))
		? SECSuccess : SECFailure;
	}
    }
    offset = name->len - constraint->len;
    if (PL_strncasecmp(name->data + offset, constraint->data, constraint->len))
        return SECFailure;
    if (constraint->data[0] == '.')
        return SECSuccess;
    if (offset > 0 && name->data[offset - 1] == '@')
        return SECSuccess;
    return SECFailure;
}

/* name contains either a 4 byte IPv4 address or a 16 byte IPv6 address.
** constraint contains an address of the same length, and a subnet mask
** of the same length.  Compare name's address to the constraint's 
** address, subject to the mask.
** Return SECSuccess if they match, SECFailure if they don't. 
*/
static SECStatus
compareIPaddrN2C(const SECItem *name, const SECItem *constraint)
{
    int i;
    if (name->len == 4 && constraint->len == 8) { /* ipv4 addr */
        for (i = 0; i < 4; i++) {
	    if ((name->data[i] ^ constraint->data[i]) & constraint->data[i+4])
	        goto loser;
	}
	return SECSuccess;
    }
    if (name->len == 16 && constraint->len == 32) { /* ipv6 addr */
        for (i = 0; i < 16; i++) {
	    if ((name->data[i] ^ constraint->data[i]) & constraint->data[i+16])
	        goto loser;
	}
	return SECSuccess;
    }
loser:
    return SECFailure;
}

/* start with a SECItem that points to a URI.  Parse it lookingg for 
** a hostname.  Modify item->data and item->len to define the hostname,
** but do not modify and data at item->data.  
** If anything goes wrong, the contents of *item are undefined.
*/
static SECStatus
parseUriHostname(SECItem * item)
{
    int i;
    PRBool found = PR_FALSE;
    for (i = 0; (unsigned)(i+2) < item->len; ++i) {
	if (item->data[i  ] == ':' &&
	    item->data[i+1] == '/' &&
	    item->data[i+2] == '/') {
	    i += 3;
	    item->data += i;
	    item->len  -= i;
	    found = PR_TRUE;
	    break;
	}
    }
    if (!found) 
        return SECFailure;
    /* now look for a '/', which is an upper bound in the end of the name */
    for (i = 0; (unsigned)i < item->len; ++i) {
	if (item->data[i] == '/') {
	    item->len = i;
	    break;
	}
    }
    /* now look for a ':', which marks the end of the name */
    for (i = item->len; --i >= 0; ) {
        if (item->data[i] == ':') {
	    item->len = i;
	    break;
	}
    }
    /* now look for an '@', which marks the beginning of the hostname */
    for (i = 0; (unsigned)i < item->len; ++i) {
	if (item->data[i] == '@') {
	    ++i;
	    item->data += i;
	    item->len  -= i;
	    break;
	}
    }
    return item->len ? SECSuccess : SECFailure;
}

/* This function takes one name, and a list of constraints.
** It searches the constraints looking for a match.
** It returns SECSuccess if the name satisfies the constraints, i.e.,
** if excluded, then the name does not match any constraint, 
** if permitted, then the name matches at least one constraint.
** It returns SECFailure if the name fails to satisfy the constraints,
** or if some code fails (e.g. out of memory, or invalid constraint)
*/
static SECStatus
cert_CompareNameWithConstraints(CERTGeneralName     *name, 
				CERTNameConstraint  *constraints,
				PRBool              excluded)
{
    SECStatus           rv     = SECSuccess;
    SECStatus           matched = SECFailure;
    CERTNameConstraint  *current;

    PORT_Assert(constraints);  /* caller should not call with NULL */
    if (!constraints) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    current = constraints;
    do {
	rv = SECSuccess;
	matched = SECFailure;
	PORT_Assert(name->type == current->name.type);
	switch (name->type) {

	case certDNSName:
	    matched = compareDNSN2C(&name->name.other, 
	                            &current->name.name.other);
	    break;

	case certRFC822Name:
	    matched = compareRFC822N2C(&name->name.other, 
	                               &current->name.name.other);
	    break;

	case certURI:
	    {
		/* make a modifiable copy of the URI SECItem. */
		SECItem uri = name->name.other;
		/* find the hostname in the URI */
		rv = parseUriHostname(&uri);
		if (rv == SECSuccess) {
		    /* does our hostname meet the constraint? */
		    matched = compareURIN2C(&uri, &current->name.name.other);
		}
	    }
	    break;

	case certDirectoryName:
	    /* Determine if the constraint directory name is a "prefix"
	    ** for the directory name being tested. 
	    */
	  {
	    /* status defaults to SECEqual, so that a constraint with 
	    ** no AVAs will be a wildcard, matching all directory names.
	    */
	    SECComparison   status = SECEqual;
	    const CERTRDN **cRDNs = current->name.name.directoryName.rdns;  
	    const CERTRDN **nRDNs = name->name.directoryName.rdns;
	    while (cRDNs && *cRDNs && nRDNs && *nRDNs) { 
		/* loop over name RDNs and constraint RDNs in lock step */
		const CERTRDN *cRDN = *cRDNs++;
		const CERTRDN *nRDN = *nRDNs++;
		CERTAVA **cAVAs = cRDN->avas;
		while (cAVAs && *cAVAs) { /* loop over constraint AVAs */
		    CERTAVA *cAVA = *cAVAs++;
		    CERTAVA **nAVAs = nRDN->avas;
		    while (nAVAs && *nAVAs) { /* loop over name AVAs */
			CERTAVA *nAVA = *nAVAs++;
			status = CERT_CompareAVA(cAVA, nAVA);
			if (status == SECEqual) 
			    break;
		    } /* loop over name AVAs */
		    if (status != SECEqual) 
			break;
		} /* loop over constraint AVAs */
		if (status != SECEqual) 
		    break;
	    } /* loop over name RDNs and constraint RDNs */
	    matched = (status == SECEqual) ? SECSuccess : SECFailure;
	    break;
	  }

	case certIPAddress:	/* type 8 */
	    matched = compareIPaddrN2C(&name->name.other, 
	                               &current->name.name.other);
	    break;

	/* NSS does not know how to compare these "Other" type names with 
	** their respective constraints.  But it does know how to tell
	** if the constraint applies to the type of name (by comparing
	** the constraint OID to the name OID).  NSS makes no use of "Other"
	** type names at all, so NSS errs on the side of leniency for these 
	** types, provided that their OIDs match.  So, when an "Other"
	** name constraint appears in an excluded subtree, it never causes
	** a name to fail.  When an "Other" name constraint appears in a
	** permitted subtree, AND the constraint's OID matches the name's
	** OID, then name is treated as if it matches the constraint.
	*/
	case certOtherName:	/* type 1 */
	    matched = (!excluded &&
		       name->type == current->name.type &&
		       SECITEM_ItemsAreEqual(&name->name.OthName.oid,
					     &current->name.name.OthName.oid))
		 ? SECSuccess : SECFailure;
	    break;

	/* NSS does not know how to compare these types of names with their
	** respective constraints.  But NSS makes no use of these types of 
	** names at all, so it errs on the side of leniency for these types.
	** Constraints for these types of names never cause the name to 
	** fail the constraints test.  NSS behaves as if the name matched
	** for permitted constraints, and did not match for excluded ones.
	*/
	case certX400Address:	/* type 4 */
	case certEDIPartyName:  /* type 6 */
	case certRegisterID:	/* type 9 */
	    matched = excluded ? SECFailure : SECSuccess;
	    break;

	default: /* non-standard types are not supported */
	    rv = SECFailure;
	    break;
	}
	if (matched == SECSuccess || rv != SECSuccess)
	    break;
	current = CERT_GetNextNameConstraint(current);
    } while (current != constraints);
    if (rv == SECSuccess) {
        if (matched == SECSuccess) 
	    rv = excluded ? SECFailure : SECSuccess;
	else
	    rv = excluded ? SECSuccess : SECFailure;
	return rv;
    }

    return SECFailure;
}

/* Extract the name constraints extension from the CA cert.
** Test each and every name in namesList against all the constraints
** relevant to that type of name.
** Returns NULL for success, all names are acceptable.
** If some name is not acceptable, returns a pointer to the cert that
** contained that name.
*/
SECStatus
CERT_CompareNameSpace(CERTCertificate  *cert,
		      CERTGeneralName  *namesList,
 		      CERTCertificate **certsList,
 		      PRArenaPool      *arena,
 		      CERTCertificate **pBadCert)
{
    SECStatus            rv;
    SECItem              constraintsExtension;
    CERTNameConstraints  *constraints;
    CERTGeneralName      *currentName;
    int                  count = 0;
    CERTNameConstraint  *matchingConstraints;
    CERTCertificate      *badCert = NULL;
    
    rv = CERT_FindCertExtension(cert, SEC_OID_X509_NAME_CONSTRAINTS, 
                                &constraintsExtension);
    if (rv != SECSuccess) {
	if (PORT_GetError() == SEC_ERROR_EXTENSION_NOT_FOUND) {
	    rv = SECSuccess;
	} else {
	    count = -1;
	}
	goto done;
    }
    /* TODO: mark arena */
    constraints = cert_DecodeNameConstraints(arena, &constraintsExtension);
    currentName = namesList;
    if (constraints == NULL) { /* decode failed */
	rv = SECFailure;
    	count = -1;
	goto done;
    } 
    do {
 	if (constraints->excluded != NULL) {
 	    rv = CERT_GetNameConstraintByType(constraints->excluded, 
	                                      currentName->type, 
 					      &matchingConstraints, arena);
 	    if (rv == SECSuccess && matchingConstraints != NULL) {
 		rv = cert_CompareNameWithConstraints(currentName, 
		                                     matchingConstraints,
 						     PR_TRUE);
 	    }
	    if (rv != SECSuccess) 
		break;
 	}
 	if (constraints->permited != NULL) {
 	    rv = CERT_GetNameConstraintByType(constraints->permited, 
	                                      currentName->type, 
 					      &matchingConstraints, arena);
            if (rv == SECSuccess && matchingConstraints != NULL) {
 		rv = cert_CompareNameWithConstraints(currentName, 
		                                     matchingConstraints,
 						     PR_FALSE);
 	    }
	    if (rv != SECSuccess) 
		break;
 	}
 	currentName = CERT_GetNextGeneralName(currentName);
 	count ++;
    } while (currentName != namesList);
done:
    if (rv != SECSuccess) {
	badCert = (count >= 0) ? certsList[count] : cert;
    }
    if (pBadCert)
	*pBadCert = badCert;
    /* TODO: release back to mark */
    return rv;
}

/* Search the cert for an X509_SUBJECT_ALT_NAME extension.
** ASN1 Decode it into a list of alternate names.
** Search the list of alternate names for one with the NETSCAPE_NICKNAME OID.
** ASN1 Decode that name.  Turn the result into a zString.  
** Look for duplicate nickname already in the certdb. 
** If one is found, create a nickname string that is not a duplicate.
*/
char *
CERT_GetNickName(CERTCertificate   *cert,
 		 CERTCertDBHandle  *handle,
		 PRArenaPool      *nicknameArena)
{ 
    CERTGeneralName  *current;
    CERTGeneralName  *names;
    char             *nickname   = NULL;
    char             *returnName = NULL;
    char             *basename   = NULL;
    PRArenaPool      *arena      = NULL;
    CERTCertificate  *tmpcert;
    SECStatus        rv;
    int              count;
    int              found = 0;
    SECItem          altNameExtension;
    SECItem          nick;

    if (handle == NULL) {
	handle = CERT_GetDefaultCertDB();
    }
    altNameExtension.data = NULL;
    rv = CERT_FindCertExtension(cert, SEC_OID_X509_SUBJECT_ALT_NAME, 
				&altNameExtension);
    if (rv != SECSuccess) { 
	goto loser; 
    }
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	goto loser;
    }
    names = CERT_DecodeAltNameExtension(arena, &altNameExtension);
    if (names == NULL) {
	goto loser;
    } 
    current = names;
    do {
	if (current->type == certOtherName && 
	    SECOID_FindOIDTag(&current->name.OthName.oid) == 
	      SEC_OID_NETSCAPE_NICKNAME) {
	    found = 1;
	    break;
	}
	current = CERT_GetNextGeneralName(current);
    } while (current != names);
    if (!found)
    	goto loser;

    rv = SEC_ASN1DecodeItem(arena, &nick, SEC_IA5StringTemplate, 
			    &current->name.OthName.name);
    if (rv != SECSuccess) {
	goto loser;
    }

    /* make a null terminated string out of nick, with room enough at
    ** the end to add on a number of up to 21 digits in length, (a signed
    ** 64-bit number in decimal) plus a space and a "#". 
    */
    nickname = (char*)PORT_ZAlloc(nick.len + 24);
    if (!nickname) 
	goto loser;
    PORT_Strncpy(nickname, (char *)nick.data, nick.len);

    /* Don't let this cert's nickname duplicate one already in the DB.
    ** If it does, create a variant of the nickname that doesn't.
    */
    count = 0;
    while ((tmpcert = CERT_FindCertByNickname(handle, nickname)) != NULL) {
	CERT_DestroyCertificate(tmpcert);
	if (!basename) {
	    basename = PORT_Strdup(nickname);
	    if (!basename)
		goto loser;
	}
	count++;
	sprintf(nickname, "%s #%d", basename, count);
    }

    /* success */
    if (nicknameArena) {
	returnName =  PORT_ArenaStrdup(nicknameArena, nickname);
    } else {
	returnName = nickname;
	nickname = NULL;
    }
loser:
    if (arena != NULL) 
	PORT_FreeArena(arena, PR_FALSE);
    if (nickname)
	PORT_Free(nickname);
    if (basename)
	PORT_Free(basename);
    if (altNameExtension.data)
    	PORT_Free(altNameExtension.data);
    return returnName;
}

#if 0
/* not exported from shared libs, not used.  Turn on if we ever need it. */
SECStatus
CERT_CompareGeneralName(CERTGeneralName *a, CERTGeneralName *b)
{
    CERTGeneralName *currentA;
    CERTGeneralName *currentB;
    PRBool found;

    currentA = a;
    currentB = b;
    if (a != NULL) {
	do { 
	    if (currentB == NULL) {
		return SECFailure;
	    }
	    currentB = CERT_GetNextGeneralName(currentB);
	    currentA = CERT_GetNextGeneralName(currentA);
	} while (currentA != a);
    }
    if (currentB != b) {
	return SECFailure;
    }
    currentA = a;
    do {
	currentB = b;
	found = PR_FALSE;
	do {
	    if (currentB->type == currentA->type) {
		switch (currentB->type) {
		  case certDNSName:
		  case certEDIPartyName:
		  case certIPAddress:
		  case certRegisterID:
		  case certRFC822Name:
		  case certX400Address:
		  case certURI:
		    if (SECITEM_CompareItem(&currentA->name.other,
					    &currentB->name.other) 
			== SECEqual) {
			found = PR_TRUE;
		    }
		    break;
		  case certOtherName:
		    if (SECITEM_CompareItem(&currentA->name.OthName.oid,
					    &currentB->name.OthName.oid) 
			== SECEqual &&
			SECITEM_CompareItem(&currentA->name.OthName.name,
					    &currentB->name.OthName.name)
			== SECEqual) {
			found = PR_TRUE;
		    }
		    break;
		  case certDirectoryName:
		    if (CERT_CompareName(&currentA->name.directoryName,
					 &currentB->name.directoryName)
			== SECEqual) {
			found = PR_TRUE;
		    }
		}
		    
	    }
	    currentB = CERT_GetNextGeneralName(currentB);
	} while (currentB != b && found != PR_TRUE);
	if (found != PR_TRUE) {
	    return SECFailure;
	}
	currentA = CERT_GetNextGeneralName(currentA);
    } while (currentA != a);
    return SECSuccess;
}

SECStatus
CERT_CompareGeneralNameLists(CERTGeneralNameList *a, CERTGeneralNameList *b)
{
    SECStatus rv;

    if (a == b) {
	return SECSuccess;
    }
    if (a != NULL && b != NULL) {
	PZ_Lock(a->lock);
	PZ_Lock(b->lock);
	rv = CERT_CompareGeneralName(a->name, b->name);
	PZ_Unlock(a->lock);
	PZ_Unlock(b->lock);
    } else {
	rv = SECFailure;
    }
    return rv;
}
#endif

#if 0
/* This function is not exported from NSS shared libraries, and is not
** used inside of NSS.
** XXX it doesn't check for failed allocations. :-(
*/
void *
CERT_GetGeneralNameFromListByType(CERTGeneralNameList *list,
				  CERTGeneralNameType type,
				  PRArenaPool *arena)
{
    CERTName *name = NULL; 
    SECItem *item = NULL;
    OtherName *other = NULL;
    OtherName *tmpOther = NULL;
    void *data;

    PZ_Lock(list->lock);
    data = CERT_GetGeneralNameByType(list->name, type, PR_FALSE);
    if (data != NULL) {
	switch (type) {
	  case certDNSName:
	  case certEDIPartyName:
	  case certIPAddress:
	  case certRegisterID:
	  case certRFC822Name:
	  case certX400Address:
	  case certURI:
	    if (arena != NULL) {
		item = PORT_ArenaNew(arena, SECItem);
		if (item != NULL) {
XXX		    SECITEM_CopyItem(arena, item, (SECItem *) data);
		}
	    } else { 
		item = SECITEM_DupItem((SECItem *) data);
	    }
	    PZ_Unlock(list->lock);
	    return item;
	  case certOtherName:
	    other = (OtherName *) data;
	    if (arena != NULL) {
		tmpOther = PORT_ArenaNew(arena, OtherName);
	    } else {
		tmpOther = PORT_New(OtherName);
	    }
	    if (tmpOther != NULL) {
XXX		SECITEM_CopyItem(arena, &tmpOther->oid, &other->oid);
XXX		SECITEM_CopyItem(arena, &tmpOther->name, &other->name);
	    }
	    PZ_Unlock(list->lock);
	    return tmpOther;
	  case certDirectoryName:
	    if (arena) {
		name = PORT_ArenaZNew(list->arena, CERTName);
		if (name) {
XXX		    CERT_CopyName(arena, name, (CERTName *) data);
		}
	    }
	    PZ_Unlock(list->lock);
	    return name;
	}
    }
    PZ_Unlock(list->lock);
    return NULL;
}
#endif

#if 0
/* This function is not exported from NSS shared libraries, and is not
** used inside of NSS.
** XXX it should NOT be a void function, since it does allocations
** that can fail.
*/
void
CERT_AddGeneralNameToList(CERTGeneralNameList *list, 
			  CERTGeneralNameType type,
			  void *data, SECItem *oid)
{
    CERTGeneralName *name;

    if (list != NULL && data != NULL) {
	PZ_Lock(list->lock);
	name = cert_NewGeneralName(list->arena, type);
	if (!name)
	    goto done;
	switch (type) {
	  case certDNSName:
	  case certEDIPartyName:
	  case certIPAddress:
	  case certRegisterID:
	  case certRFC822Name:
	  case certX400Address:
	  case certURI:
XXX	    SECITEM_CopyItem(list->arena, &name->name.other, (SECItem *)data);
	    break;
	  case certOtherName:
XXX	    SECITEM_CopyItem(list->arena, &name->name.OthName.name,
			     (SECItem *) data);
XXX	    SECITEM_CopyItem(list->arena, &name->name.OthName.oid,
			     oid);
	    break;
	  case certDirectoryName:
XXX	    CERT_CopyName(list->arena, &name->name.directoryName,
			  (CERTName *) data);
	    break;
	}
	list->name = cert_CombineNamesLists(list->name, name);
	list->len++;
done:
	PZ_Unlock(list->lock);
    }
    return;
}
#endif
