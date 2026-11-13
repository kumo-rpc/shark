package com.intellij.protobuf.lang.psi.impl;

import com.intellij.lang.ASTNode;
import com.intellij.protobuf.lang.psi.PbFieldLabel;
import com.intellij.protobuf.lang.psi.PbListField;
import com.intellij.protobuf.lang.psi.PbNumberValue;
import com.intellij.protobuf.lang.psi.PbOptionList;
import com.intellij.protobuf.lang.psi.PbTypeName;
import com.intellij.protobuf.lang.psi.PbVisitor;
import com.intellij.psi.PsiElement;
import com.intellij.psi.PsiElementVisitor;
import com.intellij.psi.util.PsiTreeUtil;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

import static com.intellij.protobuf.lang.psi.ProtoTokenTypes.IDENTIFIER_LITERAL;

public class PbListFieldImpl extends PbFieldBase implements PbListField {

  public PbListFieldImpl(ASTNode node) {
    super(node);
  }

  public void accept(@NotNull PbVisitor visitor) {
    visitor.visitListField(this);
  }

  @Override
  public void accept(@NotNull PsiElementVisitor visitor) {
    if (visitor instanceof PbVisitor) accept((PbVisitor) visitor);
    else super.accept(visitor);
  }

  @Override
  public @NotNull CanonicalFieldLabel getCanonicalLabel() {
    return CanonicalFieldLabel.REPEATED;
  }

  @Override
  @Nullable
  public PbOptionList getOptionList() {
    return PsiTreeUtil.getChildOfType(this, PbOptionList.class);
  }

  @Override
  @NotNull
  public PbTypeName getTypeName() {
    return notNullChild(PsiTreeUtil.getChildOfType(this, PbTypeName.class));
  }

  @Override
  @Nullable
  public PsiElement getNameIdentifier() {
    PsiElement name = null;
    for (PsiElement child = getFirstChild(); child != null; child = child.getNextSibling()) {
      if (child.getNode().getElementType() == IDENTIFIER_LITERAL
          && !"list".equals(child.getText())) {
        name = child;
      }
    }
    return name;
  }

  @Override
  @Nullable
  public PbNumberValue getFieldNumber() {
    return PsiTreeUtil.getChildOfType(this, PbNumberValue.class);
  }

  @Override
  @Nullable
  public PbFieldLabel getDeclaredLabel() {
    return PsiTreeUtil.getChildOfType(this, PbFieldLabel.class);
  }
}
