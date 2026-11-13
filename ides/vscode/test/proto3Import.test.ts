import * as assert from 'assert';

// Test the regex pattern directly since Proto3Import module depends on vscode
// This is the same regex from proto3Import.ts
const importStatementRegex = /^\s*import\s+('|")(.+\.shark)('|")\s*;\s*$/gim;

// Helper to reset regex and test
function testRegex(text: string): RegExpExecArray | null {
  importStatementRegex.lastIndex = 0;
  return importStatementRegex.exec(text);
}

function findAllMatches(text: string): string[] {
  const matches: string[] = [];
  let match: RegExpExecArray | null;
  importStatementRegex.lastIndex = 0;
  while ((match = importStatementRegex.exec(text)) !== null) {
    matches.push(match[2]);
  }
  return matches;
}

suite('Proto3Import', () => {
  suite('importStatementRegex', () => {
    test('matches basic import with double quotes', () => {
      const match = testRegex('import "foo.shark";');
      assert.ok(match);
      assert.strictEqual(match[2], 'foo.shark');
    });

    test('matches basic import with single quotes', () => {
      const match = testRegex("import 'foo.shark';");
      assert.ok(match);
      assert.strictEqual(match[2], 'foo.shark');
    });

    test('matches import with path', () => {
      const match = testRegex('import "shark/types/timestamp.shark";');
      assert.ok(match);
      assert.strictEqual(match[2], 'shark/types/timestamp.shark');
    });

    test('matches import with leading whitespace', () => {
      const match = testRegex('  import "foo.shark";');
      assert.ok(match);
      assert.strictEqual(match[2], 'foo.shark');
    });

    test('matches import with trailing whitespace', () => {
      const match = testRegex('import "foo.shark";   ');
      assert.ok(match);
    });

    test('matches import with spaces around path', () => {
      const match = testRegex('import "foo.shark" ;');
      assert.ok(match);
    });

    test('does not match import without semicolon', () => {
      const match = testRegex('import "foo.shark"');
      assert.strictEqual(match, null);
    });

    test('does not match non-proto imports', () => {
      const match = testRegex('import "foo.txt";');
      assert.strictEqual(match, null);
    });

    test('is case insensitive', () => {
      const match = testRegex('IMPORT "foo.shark";');
      assert.ok(match);
    });

    test('matches multiple imports in text', () => {
      const text = `import "a.shark";\nimport "b.shark";\nimport "c.shark";`;
      const matches = findAllMatches(text);
      assert.deepStrictEqual(matches, ['a.shark', 'b.shark', 'c.shark']);
    });

    test('matches imports with nested paths', () => {
      const match = testRegex('import "very/deep/nested/path/file.shark";');
      assert.ok(match);
      assert.strictEqual(match[2], 'very/deep/nested/path/file.shark');
    });

    test('matches imports with underscores in filename', () => {
      const match = testRegex('import "my_proto_file.shark";');
      assert.ok(match);
      assert.strictEqual(match[2], 'my_proto_file.shark');
    });

    test('matches imports with numbers in filename', () => {
      const match = testRegex('import "v1/api_v2.shark";');
      assert.ok(match);
      assert.strictEqual(match[2], 'v1/api_v2.shark');
    });

    test('matches imports with hyphens in path', () => {
      const match = testRegex('import "my-package/file.shark";');
      assert.ok(match);
      assert.strictEqual(match[2], 'my-package/file.shark');
    });
  });

  suite('Edge Cases', () => {
    test('handles mixed quote styles in document', () => {
      const text = `import "a.shark";\nimport 'b.shark';`;
      const matches = findAllMatches(text);
      assert.deepStrictEqual(matches, ['a.shark', 'b.shark']);
    });

    test('handles imports mixed with other statements', () => {
      const text = `syntax = "proto2";\n\nimport "foo.shark";\n\npackage test;\n\nimport "bar.shark";`;
      const matches = findAllMatches(text);
      assert.deepStrictEqual(matches, ['foo.shark', 'bar.shark']);
    });

    test('handles Windows-style paths (backslashes may not match)', () => {
      // Note: Proto files typically use forward slashes even on Windows
      const match = testRegex('import "folder/subfolder/file.shark";');
      assert.ok(match);
    });

    test('handles dots in directory names', () => {
      const match = testRegex('import "google.api/annotations.shark";');
      assert.ok(match);
      assert.strictEqual(match[2], 'google.api/annotations.shark');
    });

    test('handles public and weak imports', () => {
      // Note: The regex doesn't currently support public/weak keywords
      // This test documents the current behavior
      const publicMatch = testRegex('import public "foo.shark";');
      // Current regex doesn't match public imports - this is expected behavior
      assert.strictEqual(publicMatch, null);
    });

    test('does not match commented out imports', () => {
      // Single line - the regex matches the whole line, so comments after would still match
      const match = testRegex('// import "foo.shark";');
      // Since there's no import at line start (// prefix), it won't match
      assert.strictEqual(match, null);
    });

    test('handles empty path (edge case)', () => {
      // Import with just ".shark" doesn't actually match the regex
      // because \.shark needs a character before it
      const match = testRegex('import ".shark";');
      // This doesn't match - the regex requires chars before .shark
      assert.strictEqual(match, null);
    });

    test('handles proto with additional extension', () => {
      const match = testRegex('import "file.shark.bak";');
      // Doesn't match because it doesn't end with .shark
      assert.strictEqual(match, null);
    });
  });
});
