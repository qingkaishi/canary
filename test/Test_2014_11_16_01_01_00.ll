; --dot-dyck-callgraph
; ModuleID = 'test.bc'
target datalayout = "e-m:e-p:32:32-f64:32:64-f80:32-n8:16:32-S128"
target triple = "i386-pc-linux-gnu"

%union.pthread_attr_t = type { i32, [32 x i8] }

@start_routine = common global i8* (i8*)* null, align 4

; Function Attrs: nounwind
define i8* @t(i8* %args) #0 {
entry:
  %args.addr = alloca i8*, align 4
  store i8* %args, i8** %args.addr, align 4
  ret i8* null
}

; Function Attrs: nounwind
define void @f() #0 {
entry:
  store i8* (i8*)* @t, i8* (i8*)** @start_routine, align 4
  ret void
}

; Function Attrs: nounwind
define i32 @main() #0 {
entry:
  %retval = alloca i32, align 4
  %tid = alloca i32, align 4
  store i32 0, i32* %retval
  call void @f()
  %0 = load i8* (i8*)** @start_routine, align 4
  %call = call i32 @pthread_create(i32* %tid, %union.pthread_attr_t* null, i8* (i8*)* %0, i8* null) #1
  ret i32 0
}

; Function Attrs: nounwind
declare i32 @pthread_create(i32*, %union.pthread_attr_t*, i8* (i8*)*, i8*) #0

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind }

!llvm.ident = !{!0}

!0 = metadata !{metadata !"clang version 3.6.0 (https://github.com/llvm-mirror/clang.git 5b0b279f796ecf91b10ba8b0ca89f9dbf802bae4) (https://github.com/llvm-mirror/llvm.git 75318bcc3c15319fce936c3d45b440925998455c)"}
