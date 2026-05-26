savedcmd_signed_module.mod := printf '%s\n'   signed_module.o | awk '!x[$$0]++ { print("./"$$0) }' > signed_module.mod
