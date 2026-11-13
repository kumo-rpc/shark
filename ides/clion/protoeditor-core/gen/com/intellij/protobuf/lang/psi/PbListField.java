package com.intellij.protobuf.lang.psi;

import com.intellij.psi.PsiElement;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

public interface PbListField extends PbField {

  @Nullable
  PbOptionList getOptionList();

  @NotNull
  PbTypeName getTypeName();

  @Nullable
  PsiElement getNameIdentifier();

  @Nullable
  PbNumberValue getFieldNumber();

  @Nullable
  PbFieldLabel getDeclaredLabel();
}
