;; Include the custom character
;; charset 2bpp - Coco 1/2 - Can be edited with any 2bpp editor
;; I used https://github.com/matosimi/atari-fontmaker/releases/tag/V1.6.17.4
;;
;; charset-16.img.bin - 4bpp - Coco 3 - Created with Aseprite converted with grit
;; grit command: grit charset-16.png -gt -gu8 -gB4 -p! -ftb
        SECTION rodata
        EXPORT _charset
_charset
        INCLUDEBIN ../../support/coco/charset.bin
        ENDSECTION