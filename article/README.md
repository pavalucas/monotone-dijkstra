# Article

Submission for the SBC Maratona Revista (https://maratona.sbc.org.br/revista.html).

Based on the official template
(https://maratona.sbc.org.br/revista/revista-maratona-template.zip): the preamble
in `main.tex` is kept verbatim; only the title, authors, abstract, body
(`article.tex`) and `references.bib` were replaced.

## Files
- `main.tex` — preamble, title/abstract, includes `article.tex` and the bibliography.
- `article.tex` — body sections (skeleton with `% TODO` markers per writing block).
- `references.bib` — Costa et al. 2025, Ahuja et al. 1990, Dial 1969.

## Build
```bash
pdflatex main.tex
bibtex main
pdflatex main.tex
pdflatex main.tex
```
Or upload the `article/` folder to Overleaf.
