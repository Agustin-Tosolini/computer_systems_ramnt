; file: convert.asm
segment .text
        global  convert
convert:
        enter   4,0                     ; Stack Frame
        
        mov eax, [ebp+8]
        mov dword [ebp-4], eax         
        
        fld dword [ebp-4]           
        fistp dword [ebp-4]         
        
        mov eax, [ebp-4]            
        inc eax                     
;
; [Fin del bloque modificado]
;
        leave                     
        ret