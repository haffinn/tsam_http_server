
.DEFAULT: answers
.PHONY: answers delivery handin dist check

HOMEWORK = pa2
COURSE = tsam15

-include /labs/${COURSE}/Makefile.include

answers:
	@echo No questions

delivery handin:
	@echo Submitting answers.
	@rm -f /labs/${COURSE}/.handin/${HOMEWORK}/${USER}/handin.tar.gz
	@make -C src distclean
	@tar czf /labs/${COURSE}/.handin/${HOMEWORK}/${USER}/handin.tar.gz src
	@echo
	@make check

check:
	@if test -f /labs/${COURSE}/.handin/${HOMEWORK}/${USER}/handin.tar.gz; then echo Handin file is there. ; else echo Handin file is missing; fi
	@if test -f /labs/${COURSE}/.handin/${HOMEWORK}/${USER}/handin.tar.gz; then echo File last submitted at $$(stat -c "%y" /labs/${COURSE}/.handin/${HOMEWORK}/${USER}/handin.tar.gz) ; fi
	@if test -f /labs/${COURSE}/.handin/${HOMEWORK}/${USER}/handin.tar.gz; then echo File digest is $$(openssl dgst -sha256 -hex /labs/${COURSE}/.handin/${HOMEWORK}/${USER}/handin.tar.gz) ; fi

dist:
	test -d ${COURSE} || mkdir ${COURSE}
	test -d ${COURSE}/${HOMEWORK} || mkdir ${COURSE}/${HOMEWORK}
	test -d ${COURSE}/${HOMEWORK}/src || mkdir -p ${COURSE}/${HOMEWORK}/src
	test -d ${COURSE}/${HOMEWORK}/data || mkdir -p ${COURSE}/${HOMEWORK}/data
	install -m 0664 Makefile ${COURSE}/${HOMEWORK}/Makefile
	install -m 0664 src/Makefile ${COURSE}/${HOMEWORK}/src/Makefile
	install -m 0664 src/AUTHORS ${COURSE}/${HOMEWORK}/src/AUTHORS
	install -m 0664 src/README ${COURSE}/${HOMEWORK}/src/README
	install -m 0664 src/httpd.c ${COURSE}/${HOMEWORK}/src/httpd.c
	tar cvzf ${HOMEWORK}.tar ${COURSE}/${HOMEWORK}/data ${COURSE}/${HOMEWORK}/Makefile ${COURSE}/${HOMEWORK}/src/Makefile ${COURSE}/${HOMEWORK}/src/README ${COURSE}/${HOMEWORK}/src/AUTHORS ${COURSE}/${HOMEWORK}/src/httpd.c
