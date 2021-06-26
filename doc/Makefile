.SUFFIXES: .lisp .fasl .tex .pdf

.tex.pdf:
	xelatex -halt-on-error -interaction=batchmode ${.IMPSRC}
	bibtex ${.PREFIX}.aux

a-fork-in-the-road.pdf: a-fork-in-the-road.tex references.bib
shell-game.pdf: shell-game.tex references.bib

clean:
	rm *.aux *.bbl *.blg *.log *.out

.PHONY: clean
