/* 
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
 * The Original Code is the elliptic curve math library.
 *
 * The Initial Developer of the Original Code is
 * Sun Microsystems, Inc.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Stephen Fung <fungstep@hotmail.com> and
 *   Douglas Stebila <douglas@stebila.ca>, Sun Microsystems Laboratories
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

#ifndef __ecl_priv_h_
#define __ecl_priv_h_

#include "ecl.h"
#include "mpi.h"
#include "mplogic.h"

/* MAX_FIELD_SIZE_DIGITS is the maximum size of field element supported */
#if defined(MP_USE_LONG_LONG_DIGIT) || defined(MP_USE_LONG_DIGIT)
#define ECL_SIXTY_FOUR_BIT
#define ECL_BITS 64
#define ECL_MAX_FIELD_SIZE_DIGITS 10
#else
#define ECL_THIRTY_TWO_BIT
#define ECL_BITS 32
#define ECL_MAX_FIELD_SIZE_DIGITS 20
#endif

/* Gets the i'th bit in the binary representation of a. If i >= length(a), 
 * then return 0. (The above behaviour differs from mpl_get_bit, which
 * causes an error if i >= length(a).) */
#define MP_GET_BIT(a, i) \
	((i) >= mpl_significant_bits((a))) ? 0 : mpl_get_bit((a), (i))

struct GFMethodStr;
typedef struct GFMethodStr GFMethod;
struct GFMethodStr {
	/* Indicates whether the structure was constructed from dynamic memory 
	 * or statically created. */
	int constructed;
	/* Irreducible that defines the field. For prime fields, this is the
	 * prime p. For binary polynomial fields, this is the bitstring
	 * representation of the irreducible polynomial. */
	mp_int irr;
	/* For prime fields, the value irr_arr[0] is the number of bits in the 
	 * field. For binary polynomial fields, the irreducible polynomial
	 * f(t) is represented as an array of unsigned int[], where f(t) is
	 * of the form: f(t) = t^p[0] + t^p[1] + ... + t^p[4] where m = p[0]
	 * > p[1] > ... > p[4] = 0. */
	unsigned int irr_arr[5];
	/* Field arithmetic methods. All methods (except field_enc and
	 * field_dec) are assumed to take field-encoded parameters and return
	 * field-encoded values. All methods (except field_enc and field_dec)
	 * are required to be implemented. */
	mp_err (*field_add) (const mp_int *a, const mp_int *b, mp_int *r,
						 const GFMethod *meth);
	mp_err (*field_neg) (const mp_int *a, mp_int *r, const GFMethod *meth);
	mp_err (*field_sub) (const mp_int *a, const mp_int *b, mp_int *r,
						 const GFMethod *meth);
	mp_err (*field_mod) (const mp_int *a, mp_int *r, const GFMethod *meth);
	mp_err (*field_mul) (const mp_int *a, const mp_int *b, mp_int *r,
						 const GFMethod *meth);
	mp_err (*field_sqr) (const mp_int *a, mp_int *r, const GFMethod *meth);
	mp_err (*field_div) (const mp_int *a, const mp_int *b, mp_int *r,
						 const GFMethod *meth);
	mp_err (*field_enc) (const mp_int *a, mp_int *r, const GFMethod *meth);
	mp_err (*field_dec) (const mp_int *a, mp_int *r, const GFMethod *meth);
	/* Extra storage for implementation-specific data.  Any memory
	 * allocated to these extra fields will be cleared by extra_free. */
	void *extra1;
	void *extra2;
	void (*extra_free) (GFMethod *meth);
};

/* Construct generic GFMethods. */
GFMethod *GFMethod_consGFp(const mp_int *irr);
GFMethod *GFMethod_consGFp_mont(const mp_int *irr);
GFMethod *GFMethod_consGF2m(const mp_int *irr,
							const unsigned int irr_arr[5]);
/* Free the memory allocated (if any) to a GFMethod object. */
void GFMethod_free(GFMethod *meth);

struct ECGroupStr {
	/* Indicates whether the structure was constructed from dynamic memory 
	 * or statically created. */
	int constructed;
	/* Field definition and arithmetic. */
	GFMethod *meth;
	/* Textual representation of curve name, if any. */
	char *text;
	/* Curve parameters, field-encoded. */
	mp_int curvea, curveb;
	/* x and y coordinates of the base point, field-encoded. */
	mp_int genx, geny;
	/* Order and cofactor of the base point. */
	mp_int order;
	int cofactor;
	/* Point arithmetic methods. All methods are assumed to take
	 * field-encoded parameters and return field-encoded values. All
	 * methods (except base_point_mul and points_mul) are required to be
	 * implemented. */
	mp_err (*point_add) (const mp_int *px, const mp_int *py,
						 const mp_int *qx, const mp_int *qy, mp_int *rx,
						 mp_int *ry, const ECGroup *group);
	mp_err (*point_sub) (const mp_int *px, const mp_int *py,
						 const mp_int *qx, const mp_int *qy, mp_int *rx,
						 mp_int *ry, const ECGroup *group);
	mp_err (*point_dbl) (const mp_int *px, const mp_int *py, mp_int *rx,
						 mp_int *ry, const ECGroup *group);
	mp_err (*point_mul) (const mp_int *n, const mp_int *px,
						 const mp_int *py, mp_int *rx, mp_int *ry,
						 const ECGroup *group);
	mp_err (*base_point_mul) (const mp_int *n, mp_int *rx, mp_int *ry,
							  const ECGroup *group);
	mp_err (*points_mul) (const mp_int *k1, const mp_int *k2,
						  const mp_int *px, const mp_int *py, mp_int *rx,
						  mp_int *ry, const ECGroup *group);
	/* Extra storage for implementation-specific data.  Any memory
	 * allocated to these extra fields will be cleared by extra_free. */
	void *extra1;
	void *extra2;
	void (*extra_free) (ECGroup *group);
};

/* Wrapper functions for generic prime field arithmetic. */
mp_err ec_GFp_add(const mp_int *a, const mp_int *b, mp_int *r,
				  const GFMethod *meth);
mp_err ec_GFp_neg(const mp_int *a, mp_int *r, const GFMethod *meth);
mp_err ec_GFp_sub(const mp_int *a, const mp_int *b, mp_int *r,
				  const GFMethod *meth);
mp_err ec_GFp_mod(const mp_int *a, mp_int *r, const GFMethod *meth);
mp_err ec_GFp_mul(const mp_int *a, const mp_int *b, mp_int *r,
				  const GFMethod *meth);
mp_err ec_GFp_sqr(const mp_int *a, mp_int *r, const GFMethod *meth);
mp_err ec_GFp_div(const mp_int *a, const mp_int *b, mp_int *r,
				  const GFMethod *meth);
/* Wrapper functions for generic binary polynomial field arithmetic. */
mp_err ec_GF2m_add(const mp_int *a, const mp_int *b, mp_int *r,
				   const GFMethod *meth);
mp_err ec_GF2m_neg(const mp_int *a, mp_int *r, const GFMethod *meth);
mp_err ec_GF2m_mod(const mp_int *a, mp_int *r, const GFMethod *meth);
mp_err ec_GF2m_mul(const mp_int *a, const mp_int *b, mp_int *r,
				   const GFMethod *meth);
mp_err ec_GF2m_sqr(const mp_int *a, mp_int *r, const GFMethod *meth);
mp_err ec_GF2m_div(const mp_int *a, const mp_int *b, mp_int *r,
				   const GFMethod *meth);

/* Montgomery prime field arithmetic. */
mp_err ec_GFp_mul_mont(const mp_int *a, const mp_int *b, mp_int *r,
					   const GFMethod *meth);
mp_err ec_GFp_sqr_mont(const mp_int *a, mp_int *r, const GFMethod *meth);
mp_err ec_GFp_div_mont(const mp_int *a, const mp_int *b, mp_int *r,
					   const GFMethod *meth);
mp_err ec_GFp_enc_mont(const mp_int *a, mp_int *r, const GFMethod *meth);
mp_err ec_GFp_dec_mont(const mp_int *a, mp_int *r, const GFMethod *meth);
void ec_GFp_extra_free_mont(GFMethod *meth);

/* point multiplication */
mp_err ec_pts_mul_basic(const mp_int *k1, const mp_int *k2,
						const mp_int *px, const mp_int *py, mp_int *rx,
						mp_int *ry, const ECGroup *group);
mp_err ec_pts_mul_simul_w2(const mp_int *k1, const mp_int *k2,
						   const mp_int *px, const mp_int *py, mp_int *rx,
						   mp_int *ry, const ECGroup *group);

/* Computes the windowed non-adjacent-form (NAF) of a scalar. Out should
 * be an array of signed char's to output to, bitsize should be the number 
 * of bits of out, in is the original scalar, and w is the window size.
 * NAF is discussed in the paper: D. Hankerson, J. Hernandez and A.
 * Menezes, "Software implementation of elliptic curve cryptography over
 * binary fields", Proc. CHES 2000. */
mp_err ec_compute_wNAF(signed char *out, int bitsize, const mp_int *in,
					   int w);

/* Optimized field arithmetic */
mp_err ec_group_set_gfp192(ECGroup *group, ECCurveName);
mp_err ec_group_set_gfp224(ECGroup *group, ECCurveName);
mp_err ec_group_set_gf2m163(ECGroup *group, ECCurveName name);
mp_err ec_group_set_gf2m193(ECGroup *group, ECCurveName name);
mp_err ec_group_set_gf2m233(ECGroup *group, ECCurveName name);

/* Optimized floating-point arithmetic */
#ifdef ECL_USE_FP
mp_err ec_group_set_secp160r1_fp(ECGroup *group);
mp_err ec_group_set_nistp192_fp(ECGroup *group);
mp_err ec_group_set_nistp224_fp(ECGroup *group);
#endif

#endif							/* __ecl_priv_h_ */
