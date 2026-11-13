/*
 * Copyright 2019 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.intellij.protobuf.lang.psi.impl;

import com.intellij.lang.ASTNode;
import com.intellij.protobuf.ide.util.PbIcons;
import com.intellij.protobuf.lang.descriptor.Descriptor;
import com.intellij.protobuf.lang.descriptor.DescriptorOptionType;
import com.intellij.protobuf.lang.psi.PbEnumDefinition;
import com.intellij.protobuf.lang.psi.PbEnumValue;
import com.intellij.protobuf.lang.psi.PbOptionExpression;
import com.intellij.protobuf.lang.psi.PbOptionList;
import com.intellij.protobuf.lang.psi.util.PbPsiUtil;
import com.intellij.psi.util.PsiTreeUtil;
import com.intellij.psi.util.QualifiedName;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

import javax.swing.Icon;
import java.util.Collections;
import java.util.List;

abstract class PbEnumValueMixin extends PbNamedElementBase implements PbEnumValue {

  PbEnumValueMixin(ASTNode node) {
    super(node);
  }

  @Override
  public @NotNull QualifiedName getDescriptorOptionsTypeName(Descriptor descriptor) {
    return DescriptorOptionType.ENUM_VALUE_OPTIONS.forDescriptor(descriptor);
  }

  @Override
  public @Nullable Icon getIcon(int flags) {
    return PbIcons.ENUM_VALUE;
  }

  @Override
  public @NotNull List<PbOptionExpression> getOptions() {
    PbOptionList optionList = getOptionList();
    if (optionList == null) {
      return Collections.emptyList();
    }
    return optionList.getOptions();
  }

  @Override
  public @Nullable QualifiedName getExtensionOptionScope() {
    QualifiedName name = getQualifiedName();
    if (name == null) {
      return null;
    }
    // For an `enum class`, the value's qualified name is `pkg.EnumName.ValueName` (the enum
    // introduces a child scope). The enum-value extension option scope is the enum's own
    // package scope (`pkg`), matching PbEnumBodyMixin.getExtensionOptionScope, so we strip both
    // the value name and the enum name. For a plain `enum`, the value's qualified name is
    // `pkg.ValueName` and stripping the value name yields the package scope directly.
    PbEnumDefinition enumDefinition = PsiTreeUtil.getParentOfType(this, PbEnumDefinition.class);
    if (enumDefinition != null && PbPsiUtil.isEnumClass(enumDefinition)) {
      QualifiedName withoutEnum = name.removeLastComponent();
      return withoutEnum != null ? withoutEnum.removeLastComponent() : null;
    }
    return name.removeLastComponent();
  }
}
