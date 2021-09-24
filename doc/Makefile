.POSIX:
.SUFFIXES: .tex .pdf .md .html
.PHONY: clean all

LATEX = pdflatex
TEXFLAGS = -no-shell-escape -halt-on-error -file-line-error
BIBTEX = bibtex

all: document.pdf NOTES.html thesis.pdf

clean:
	rm -rf document.aux document.bbl document.blg document.log document.out document.toc document.lof document.lot \
	thesis.aux thesis.bbl thesis.blg thesis.log thesis.out thesis.toc thesis.lof thesis.lot

.tex.pdf:
	$(LATEX) $(TEXFLAGS) $* \
	&& $(BIBTEX) $* \
	&& $(LATEX) $(TEXFLAGS) $* \
	&& $(LATEX) $(TEXFLAGS) $*
.md.html:
	pandoc -f markdown_strict -t html5 -M title=$* \
		-s -o $@ $<
