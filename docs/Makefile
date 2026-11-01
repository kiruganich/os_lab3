.PHONY: all clean
all: report.pdf

# always rebuild, included files may have changed
.PHONY: report.pdf
report.pdf:
	pdflatex -interaction=nonstopmode -halt-on-error report.tex
	pdflatex -interaction=nonstopmode -halt-on-error report.tex

clean:
	rm -f report.aux report.log report.out report.toc report.fls report.fdb_latexmk report.synctex.gz
