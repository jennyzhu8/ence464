################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
FreeRTOS/portable/MemMang/%.obj: ../FreeRTOS/portable/MemMang/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs1210/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me -O2 --include_path="P:/ENCE464/Program1" --include_path="C:/ti/TivaWare_C_Series-2.2.0.295" --include_path="P:/ENCE464/Program1/FreeRTOS/include" --include_path="P:/ENCE464/Program1/FreeRTOS/portable/CCS/ARM_CM4F" --include_path="C:/ti/ccs1210/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/include" --define=ccs="ccs" --define=TARGET_IS_TM4C123_RB2 --define=PART_TM4C123GH6PM --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="FreeRTOS/portable/MemMang/$(basename $(<F)).d_raw" --obj_directory="FreeRTOS/portable/MemMang" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


