;;
;; $XORP: xorp/devnotes/kdoc.el,v 1.9 2002/10/24 09:16:00 pavlin Exp $
;;
;; This file is a modification of the original gnome-doc.el
;; The original copyright message is included below.
;; 

;; Copyright (C) 1998 Michael Zucchi

;; This program is free software; you can redistribute it and/or
;; modify it under the terms of the GNU General Public License as
;; published by the Free Software Foundation; either version 2 of
;; the License, or (at your option) any later version.

;; This program is distributed in the hope that it will be
;; useful, but WITHOUT ANY WARRANTY; without even the implied
;; warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
;; PURPOSE.  See the GNU General Public License for more details.

;; You should have received a copy of the GNU General Public
;; License along with this program; if not, write to the Free
;; Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
;; MA 0211
;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;  This file auto-generates a C or C++ function document header from the
;;  current function.

;;  Load into emacs and use C-x5, or M-x kdoc-insert to insert
;;  a new header for the current function.  You have to be somewhere in
;;  the body of the function you wish to document.

;;  The default header format is setup to do 'kdoc' style headers.
;;  These headers are used in the KDE project to automatically
;;  produce SGML API documentation directly from the source code.

;;  These headers look something like this (the cursor is left at
;;  the '_'):

;;  /**
;;   * Do foo
;;   *
;;   * Do foo with ...
;;   *
;;   * @param param1 the parameter that ...
;;   * @param param2 the parameter that ...
;;   * @return foo
;;   */
;;  int function_name(char *param1, int param2)
;;  {
;;  ...

;;  It should be able to transform this into quite a few different
;;  header formats by customising the options.

;;  For example, for a more gnu-ish header style, the following
;;  settings could be used:

;;  (setq-default kdoc-header "/* %s\n")
;; or for no function name in the header:
;;      (setq-default kdoc-header "/* \n")

;;  (setq-default kdoc-blank "   \n")
;; or for more compact version:
;;      (setq-default kdoc-blank "")

;;  (setq-default kdoc-parameter "   %s \n")
;; or to exclude parameters:
;;       (setq-default kdoc-parameter "")

;;  (setq-default kdoc-trailer "   */\n")
;;  (setq-default kdoc-section "   %s \n")
;;  (setq-default kdoc-match-block "^  ")
;;  (setq-default kdoc-match-header "^/\\*")

;; The important thing is to ensure all lines match
;; the kdoc-match-block regular expression.

;; This will produce something like this:

;;  /**
;;   * 
;;   *
;;   * 
;;   *
;;   * @param param1
;;   * @param param2
;;   * @return 
;;   */

;; With the blank line defined as "", a much more
;; compact version is generated:

;;  /**
;;   * @param param1
;;   * @param param2
;;   * @return 
;;   */

;;;;
;; This is my first attempt at anything remotely lisp-like, you'll just
;; have to live with it :)
;;
;; It works ok with emacs-20, AFAIK it should work on other versions too.

(defgroup kdoc nil 
  "Generates automatic headers for functions"
  :prefix "kdoc"
  :group 'tools)

(defcustom kdoc-header "/**\n * \n"
  "Start of documentation header.

Using '%s' will expand to the name of the function."
  :type '(string)
  :group 'kdoc)

(defcustom kdoc-h-header "    /**\n     * \n"
  "Start of documentation header.

Using '%s' will expand to the name of the function."
  :type '(string)
  :group 'kdoc)

(defcustom kdoc-class-header "/**\n * @short \n"
  "Start of documentation class header."
  :type '(string)
  :group 'kdoc)

(defcustom kdoc-blank " * \n"
  "Used to put a blank line into the header."
  :type '(string)
  :group 'kdoc)

(defcustom kdoc-h-blank "     * \n"
  "Used to put a blank line into the header."
  :type '(string)
  :group 'kdoc)

(defcustom kdoc-class-blank " * \n"
  "Used to put a blank line into the class header."
  :type '(string)
  :group 'kdoc)

(defcustom kdoc-trailer " */\n"
  "End of documentation header.

Using '%s' will expand to the name of the function being defined."
  :type '(string)
  :group 'kdoc)

(defcustom kdoc-h-trailer "     */\n"
  "End of documentation header.

Using '%s' will expand to the name of the function being defined."
  :type '(string)
  :group 'kdoc)

(defcustom kdoc-class-trailer " */\n"
  "End of documentation class header."
  :type '(string)
  :group 'kdoc)

(defcustom kdoc-parameter " * @param %s \n"
  "Used to add another parameter to the header.

Using '%s' will be replaced with the name of the parameter."
  :type '(string)
  :group 'kdoc)

(defcustom kdoc-h-parameter "     * @param %s \n"
  "Used to add another parameter to the header.

Using '%s' will be replaced with the name of the parameter."
  :type '(string)
  :group 'kdoc)

(defcustom kdoc-section " * %s \n"
  "How to define a new section in the output.

Using '%s' is replaced with the name of the section.
Currently this will only be used to define the 'return value' field."
  :type '(string)
  :group 'kdoc)

(defcustom kdoc-h-section "     * %s \n"
  "How to define a new section in the output.

Using '%s' is replaced with the name of the section.
Currently this will only be used to define the 'return value' field."
  :type '(string)
  :group 'kdoc)

(defcustom kdoc-class-section " * %s \n"
  "How to define a new section in the output.

Using '%s' is replaced with the name of the section."
  :type '(string)
  :group 'kdoc)

(defcustom kdoc-match-block "^ \\*"
  "Regular expression which matches all lines in the header.

It should match every line produced by any of the header specifiers.
It is therefore convenient to start all header lines with a common
comment prefix"
  :type '(string)
  :group 'kdoc)

(defcustom kdoc-h-match-block "^     \\*"
  "Regular expression which matches all lines in the header.

It should match every line produced by any of the header specifiers.
It is therefore convenient to start all header lines with a common
comment prefix"
  :type '(string)
  :group 'kdoc)

(defcustom kdoc-class-match-block "^ \\*"
  "Regular expression which matches all lines in the class header.

It should match every line produced by any of the header specifiers.
It is therefore convenient to start all header lines with a common
comment prefix"
  :type '(string)
  :group 'kdoc)

(defcustom kdoc-match-header "^/\\*\\*"
  "Regular expression which matches the first line of the header.

It is used to identify if a header has already been applied to this
function.  It should match the line produced by kdoc-header, or
at least the first line of this which matches kdoc-match-block"
  :type '(string)
  :group 'kdoc)

(defcustom kdoc-h-match-header "^    /\\*\\*"
  "Regular expression which matches the first line of the header.

It is used to identify if a header has already been applied to this
function.  It should match the line produced by kdoc-h-header, or
at least the first line of this which matches kdoc-h-match-block"
  :type '(string)
  :group 'kdoc)

(defcustom kdoc-class-match-header "^/\\*\\*"
  "Regular expression which matches the first line of the class header.

It is used to identify if a header has already been applied to this
function.  It should match the line produced by kdoc-class-header, or
at least the first line of this which matches kdoc-class-match-block"
  :type '(string)
  :group 'kdoc)


(make-variable-buffer-local 'kdoc-header)
(make-variable-buffer-local 'kdoc-trailer)
(make-variable-buffer-local 'kdoc-parameter)
(make-variable-buffer-local 'kdoc-section)
(make-variable-buffer-local 'kdoc-blank)
(make-variable-buffer-local 'kdoc-match-block)
(make-variable-buffer-local 'kdoc-match-header)
(make-variable-buffer-local 'kdoc-h-header)
(make-variable-buffer-local 'kdoc-h-trailer)
(make-variable-buffer-local 'kdoc-h-parameter)
(make-variable-buffer-local 'kdoc-h-section)
(make-variable-buffer-local 'kdoc-h-blank)
(make-variable-buffer-local 'kdoc-h-match-block)
(make-variable-buffer-local 'kdoc-h-match-header)
(make-variable-buffer-local 'kdoc-class-header)
(make-variable-buffer-local 'kdoc-class-trailer)
;(make-variable-buffer-local 'kdoc-class-parameter)
(make-variable-buffer-local 'kdoc-class-section)
(make-variable-buffer-local 'kdoc-class-blank)
(make-variable-buffer-local 'kdoc-class-match-block)
(make-variable-buffer-local 'kdoc-class-match-header)


;; insert header at current location
(defun kdoc-insert-header (function)
  (insert (format kdoc-header function)))
(defun kdoc-h-insert-header (function)
  (insert (format kdoc-h-header function)))
(defun kdoc-class-insert-header ()
  (insert kdoc-class-header))

;; insert a single variable, at current location
(defun kdoc-insert-var (var)
  (insert (format kdoc-parameter var)))
(defun kdoc-h-insert-var (var)
  (insert (format kdoc-h-parameter var)))
(defun kdoc-class-insert-var (var)
  (insert (format kdoc-class-parameter var)))

;; insert a 'blank' comment line
(defun kdoc-insert-blank ()
  (insert kdoc-blank))
(defun kdoc-h-insert-blank ()
  (insert kdoc-h-blank))
(defun kdoc-class-insert-blank ()
  (insert kdoc-class-blank))

;; insert a section comment line
(defun kdoc-insert-section (section)
  (insert (format kdoc-section section)))
(defun kdoc-h-insert-section (section)
  (insert (format kdoc-h-section section)))
(defun kdoc-class-insert-section (section)
  (insert (format kdoc-class-section section)))

;; insert the end of the header
(defun kdoc-insert-footer (func)
  (insert (format kdoc-trailer func)))
(defun kdoc-h-insert-footer (func)
  (insert (format kdoc-h-trailer func)))
(defun kdoc-class-insert-footer ()
  (insert kdoc-class-trailer))

(defun kdoc-insert ()
  "Add a documentation header to the current function.
Only C/C++ function types are properly supported currently."
  (interactive)
  (let (c-insert-here (point))
    (save-excursion
      (beginning-of-defun)
      (let (c-arglist
	    c-funcname
	    (c-point (point))
	    c-comment-point
	    c-isvoid
	    c-doinsert)
	(search-backward "(")
	(forward-line -2)
	(while (or (looking-at "^$")
		   (looking-at "^ *}")
		   (looking-at "^ \\*")
		   (looking-at "^#"))
	  (forward-line 1))
	(if (or (looking-at ".*void.*(")
		(looking-at ".*void[ \t]*$"))
	    (setq c-isvoid 1))
	(save-excursion
	  (if (re-search-forward "\\([A-Za-z0-9_:~]+\\)[ \t\n]*[^(]*\\(([^)]*)\\)" c-point nil)
	      (let ((c-argstart (match-beginning 2))
		    (c-argend (match-end 2)))
		(setq c-funcname (buffer-substring (match-beginning 1) (match-end 1)))
		(goto-char c-argstart)
		(while (re-search-forward "\\([A-Za-z0-9_]*\\) *[,)=]" c-argend t)
		  (setq c-arglist
			(append c-arglist
				(list (buffer-substring (match-beginning 1) (match-end 1)))))))))
	;; find C++ class name and class method
	;; if this is a class constructor, or destructor, the function is void
	(if (string-match "::~" c-funcname)
	    (progn
	      (setq cxx-class-name (substring c-funcname 0 (match-beginning 0)))
	      (setq cxx-class-method (substring c-funcname (match-end 0)))
	      ( if (string-equal cxx-class-name cxx-class-method)
		  (setq c-isvoid 1)))
	  (if (string-match "::" c-funcname)
	      (progn
		(setq cxx-class-name (substring c-funcname 0 (match-beginning 0)))
		(setq cxx-class-method (substring c-funcname (match-end 0)))
		( if (string-equal cxx-class-name cxx-class-method)
		    (setq c-isvoid 1)))))
	
	;; see if we already have a header here ...
	(save-excursion
	  (forward-line -1)
	  (while (looking-at kdoc-match-block)
	    (forward-line -1))
	  (if (looking-at kdoc-match-header)
	      (error "Header already exists")
	    (setq c-doinsert t)))

	;; insert header
	(if c-doinsert
	    (progn
	      (kdoc-insert-header c-funcname)
	      ;; record the point of insertion
	      (setq c-insert-here (- (point) 1))

	      ;; the blank space for description
	      (kdoc-insert-blank)
	      (kdoc-insert-blank)
	      (setq kdoc-extra-blank 0)


	      ;; all arguments
	      (while c-arglist
		(setq c-arg (car c-arglist))
		(setq c-arglist (cdr c-arglist))
		(if (> (length c-arg) 0)
		    (progn
		      (if (= kdoc-extra-blank 0)
			  (progn
			    (kdoc-insert-blank)
			    (setq kdoc-extra-blank 1)))
		      (kdoc-insert-var c-arg))))
	      
	      ;; insert a return value only if we have one ...
	      (if (not c-isvoid)
		  (progn
		    (if (= kdoc-extra-blank 0)
			(progn
			  (kdoc-insert-blank)
			  (setq kdoc-extra-blank 1)))
		    (kdoc-insert-section "@return")))
	      
	      (kdoc-insert-footer c-funcname)))))
	
    ;; goto the start of the description saved above
    (goto-char c-insert-here)))


(defun kdoc-h-insert ()
  "Add a documentation header to the current function declaration.
Only C/C++ function types are properly supported currently."
  (interactive)
  (let (c-insert-here (point))
    (save-excursion
      (beginning-of-line)
      (search-forward ")");
      ;; (beginning-of-defun)
      (let (c-arglist
	    c-funcname
	    (c-point (point))
	    c-comment-point
	    c-isvoid
	    c-doinsert)
	(search-backward "(")
	(beginning-of-line)
	(if (or (looking-at ".*void.*(")
		(looking-at ".*void[ \t]*$")
		(looking-at "[ \t]*\\(virtual[ \t]+\\)?[A-Za-z]+[A-Za-z0-9_]*[ \t]*(")
		(looking-at "[ \t]*\\(virtual[ \t]+\\)?~[A-Za-z]+[A-Za-z0-9_]*[ \t]*("))
	    (setq c-isvoid 1))
	(save-excursion
	  (if (re-search-forward "\\([A-Za-z0-9_:~]+\\)[ \t\n]*[^(]*\\(([^)]*)\\)" c-point nil)
	      (let ((c-argstart (match-beginning 2))
		    (c-argend (match-end 2)))
		(setq c-funcname (buffer-substring (match-beginning 1) (match-end 1)))
		(goto-char c-argstart)
		(while (re-search-forward "\\([A-Za-z0-9_]*\\) *[,)=]" c-argend t)
		  (setq c-arglist
			(append c-arglist
				(list (buffer-substring (match-beginning 1) (match-end 1)))))))))
	
	;; see if we already have a header here ...
	(save-excursion
	  (forward-line -1)
	  (while (looking-at kdoc-h-match-block)
	    (forward-line -1))
	  (if (looking-at kdoc-h-match-header)
	      (error "Header already exists")
	    (setq c-doinsert t)))

	;; insert header
	(if c-doinsert
	    (progn
	      (kdoc-h-insert-header c-funcname)
	      ;; record the point of insertion
	      (setq c-insert-here (- (point) 1))

	      ;; the blank space for description
	      (kdoc-h-insert-blank)
	      (kdoc-h-insert-blank)
	      (setq kdoc-h-extra-blank 0)


	      ;; all arguments
	      (while c-arglist
		(setq c-arg (car c-arglist))
		(setq c-arglist (cdr c-arglist))
		(if (> (length c-arg) 0)
		    (progn
		      (if (= kdoc-h-extra-blank 0)
			  (progn
			    (kdoc-h-insert-blank)
			    (setq kdoc-h-extra-blank 1)))
		      (kdoc-h-insert-var c-arg))))
	      
	      ;; insert a return value only if we have one ...
	      (if (not c-isvoid)
		  (progn
		    (if (= kdoc-h-extra-blank 0)
			(progn
			  (kdoc-h-insert-blank)
			  (setq kdoc-h-extra-blank 1)))
		    (kdoc-h-insert-section "@return")))
	      
	      (kdoc-h-insert-footer c-funcname)))))
	
    ;; goto the start of the description saved above
    (goto-char c-insert-here)))

(defun kdoc-class-insert ()
  "Add a documentation header to the current C++ class declaration."
  (interactive)
  (let (c-insert-here (point))
    (save-excursion
      ;; (re-search-backward "^[ \t\n]*\\(\\(export[ \t\n]+\\)?\\(template[ \t\n]*\\)<.*>[ \t\n]*\\)?class[ \t\n]+")
      (re-search-backward "[ \t\n]*class[ \t\n]+")
      (beginning-of-line)
      (while (looking-at "^[ \t]*$")
	(forward-line -1))
      ;; check if a template with some of the keywords split over 1+ lines
      (if (looking-at "^[ \t]*class[ \t\n]+")
	  (progn
	    (forward-line -1)
	    (if (not (looking-at "^[ \t]*\\(export[ \t]+\\)*template[ \t]*<.*>[ \t]*$"))
		(forward-line 1))))
      
      (let ((c-point (point))
	    c-doinsert)
	;; see if we already have a header here ...
	(save-excursion
	  (forward-line -1)
	  (while (looking-at kdoc-class-match-block)
	    (forward-line -1))
	  (if (looking-at kdoc-class-match-header)
	      (error "Header already exists")
	    (setq c-doinsert t)))

	;; insert header
	(if c-doinsert
	    (progn
	      (kdoc-class-insert-header)
	      ;; record the point of insertion
	      (setq c-insert-here (- (point) 1))

	      ;; the blank space for description
	      (kdoc-class-insert-blank)
	      (kdoc-class-insert-blank)
	      (kdoc-class-insert-footer)))))
    
    ;; goto the start of the description saved above
    (goto-char c-insert-here)))


;; set global binding for this key (follows the format for
;;   creating a changelog entry ...)
(global-set-key "\C-x5h"  'kdoc-insert)
(global-set-key "\C-x6h"  'kdoc-h-insert)
(global-set-key "\C-x7h"  'kdoc-class-insert)

(provide 'kdoc)
