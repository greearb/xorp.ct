/*
 *   OSPFD routing daemon
 *   Copyright (C) 2000 by John T. Moy
 *   
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* Internal support for Opaque-LSAs.
 */

/* We don't look at the body of Opaque-LSAs.
 * Howver, if we are requested to originate an
 * Opaque-LSA, we store the body separately so
 * that it can be refreshed from local storage.
 */

class opqLSA : public LSA {
    bool adv_opq;
    byte *local_body;
    int local_blen;
    // Stored for opaque-LSA upload, in case
    // LSA is deleted before it can be uploaded
    int phyint;
    InAddr if_addr;
    aid_t a_id;
public:
    opqLSA(class SpfIfc *, class SpfArea *, LShdr *, int blen);
    virtual void reoriginate(int forced);
    virtual void parse(LShdr *hdr);
    virtual void unparse();
    virtual void build(LShdr *hdr);
    virtual void update_in_place(LSA *);
    SpfNbr *grace_lsa_parse(byte *, int, int &);
    friend class OSPF;
};

/* Holding queue for Opaque-LSAs that are to be delivered
 * to an application.
 */

class OpqHoldQ : public AVLitem {
    LsaList holdq;
 public:
    OpqHoldQ(int conn_id);
    friend class OSPF;
};
