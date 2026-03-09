#!/bin/bash
# Compile Paper 3: From Proofs to Silicon
#
# This script compiles the LaTeX document with proper reference resolution

echo "Compiling Paper 3: From Proofs to Silicon..."
echo "=============================================="
echo ""

# First pass
echo "[1/4] First pdflatex pass..."
pdflatex -interaction=nonstopmode Paper_3_Hardware_Implementation.tex > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "ERROR: First pdflatex pass failed!"
    pdflatex -interaction=nonstopmode Paper_3_Hardware_Implementation.tex
    exit 1
fi

# BibTeX
echo "[2/4] Running bibtex..."
bibtex Paper_3_Hardware_Implementation > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "WARNING: BibTeX reported issues (may be non-fatal)"
fi

# Second pass
echo "[3/4] Second pdflatex pass..."
pdflatex -interaction=nonstopmode Paper_3_Hardware_Implementation.tex > /dev/null 2>&1

# Third pass
echo "[4/4] Third pdflatex pass..."
pdflatex -interaction=nonstopmode Paper_3_Hardware_Implementation.tex > /dev/null 2>&1

# Check output
if [ -f "Paper_3_Hardware_Implementation.pdf" ]; then
    SIZE=$(stat -c%s "Paper_3_Hardware_Implementation.pdf" 2>/dev/null || stat -f%z "Paper_3_Hardware_Implementation.pdf" 2>/dev/null)
    echo ""
    echo "Compilation successful!"
    echo "   Output: Paper_3_Hardware_Implementation.pdf"
    echo "   Size: $(echo "scale=1; $SIZE/1024" | bc 2>/dev/null || echo "$SIZE") KB"
else
    echo "ERROR: PDF was not generated!"
    exit 1
fi
