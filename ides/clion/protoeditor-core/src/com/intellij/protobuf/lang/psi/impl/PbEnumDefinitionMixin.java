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

import com.google.common.collect.Multimap;
import com.intellij.lang.ASTNode;
import com.intellij.protobuf.ide.util.PbIcons;
import com.intellij.protobuf.lang.psi.PbEnumDefinition;
import com.intellij.protobuf.lang.psi.PbEnumValue;
import com.intellij.protobuf.lang.psi.PbSymbolOwner;
import com.intellij.protobuf.lang.psi.util.PbPsiImplUtil;
import com.intellij.protobuf.lang.psi.util.PbPsiUtil;
import com.intellij.protobuf.lang.stub.PbEnumDefinitionStub;
import com.intellij.psi.stubs.IStubElementType;
import com.intellij.psi.util.QualifiedName;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

import javax.swing.Icon;

abstract class PbEnumDefinitionMixin extends PbStubbedSymbolOwnerBase<PbEnumDefinitionStub>
    implements PbEnumDefinition {

  PbEnumDefinitionMixin(ASTNode node) {
    super(node);
  }

  PbEnumDefinitionMixin(PbEnumDefinitionStub stub, IStubElementType nodeType) {
    super(stub, nodeType);
  }

  @Override
  public @NotNull Multimap<String, PbEnumValue> getEnumValueMap() {
    return PbPsiImplUtil.getCachedEnumValueMap(this);
  }

  /**
   * Returns the child symbol scope for this enum definition.
   *
   * <p>For an <code>enum class</code>, the enum introduces a new scope: enum values have qualified
   * names of the form <code>pkg.EnumName.ValueName</code>, so the child scope is this enum's own
   * qualified name (mirroring {@code PbMessageDefinition} / {@code PbServiceDefinition}).
   *
   * <p>For a plain <code>enum</code> (proto2/proto3 semantics), enum values are scoped to the
   * enclosing symbol owner (the package, or the containing message), not to the enum itself. A
   * value <code>X</code> in <code>enum Bar</code> under package <code>pkg</code> has the qualified
   * name <code>pkg.X</code> (and under <code>message Foo</code>, <code>pkg.Foo.X</code>). This
   * preserves the legacy behavior of allowing bare-name references to enum values. To achieve it,
   * we return the parent scope here instead of the enum's own qualified name.
   */
  @Override
  public @Nullable QualifiedName getChildScope() {
    if (PbPsiUtil.isEnumClass(this)) {
      return getQualifiedName();
    }
    // Plain enum: delegate to the enclosing symbol owner's scope so that enum values are not
    // prefixed with the enum name. This mirrors the pre-change behavior where PbEnumDefinition
    // was not a PbSymbolOwner and value.getSymbolOwner() skipped past the enum to the package
    // (or containing message).
    PbSymbolOwner parent = getSymbolOwner();
    return parent != null ? parent.getChildScope() : super.getChildScope();
  }

  @Override
  public @Nullable Icon getIcon(int flags) {
    return PbIcons.ENUM;
  }
}
