; --dot-dyck-callgraph
; ModuleID = 'test.bc'
target datalayout = "e-m:e-p:32:32-f64:32:64-f80:32-n8:16:32-S128"
target triple = "i386-pc-linux-gnu"

@.str = private unnamed_addr constant [11 x i8] c"function1\0A\00", align 1
@.str1 = private unnamed_addr constant [11 x i8] c"function2\0A\00", align 1

; Function Attrs: nounwind
define i32 @function(i32 %c) #0 {
entry:
  %c.addr = alloca i32, align 4
  store i32 %c, i32* %c.addr, align 4
  %call = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([11 x i8]* @.str, i32 0, i32 0))
  ret i32 1
}

declare i32 @printf(i8*, ...) #1

; Function Attrs: nounwind
define void @function2(i32* %cptr) #0 {
entry:
  %cptr.addr = alloca i32*, align 4
  store i32* %cptr, i32** %cptr.addr, align 4
  %call = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([11 x i8]* @.str1, i32 0, i32 0))
  ret void
}

; Function Attrs: nounwind
define i32 @main() #0 {
entry:
  %retval = alloca i32, align 4
  %x = alloca i8*, align 4
  %y = alloca i32**, align 4
  store i32 0, i32* %retval
  store i8* bitcast (i32 (i32)* @function to i8*), i8** %x, align 4
  %0 = load i8** %x, align 4
  %1 = bitcast i8* %0 to i32**
  store i32** %1, i32*** %y, align 4
  %2 = load i32*** %y, align 4
  %3 = bitcast i32** %2 to void (...)*
  %callee.knr.cast = bitcast void (...)* %3 to void ()*
  call void %callee.knr.cast()
  ret i32 0
}

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = metadata !{metadata !"clang version 3.6.0 (https://github.com/llvm-mirror/clang.git dd525c2374900ac14f64e487d04a208fe1724f21) (https://github.com/llvm-mirror/llvm.git 84230f9a53e8f7a9c9f2144bafad4830aba7686a)"}
