"""Read-only admission and fail-closed negatives; no model/network calls."""
import sqlite3
import retrieve

with retrieve.database() as source:
    retrieve.check(source)
    with sqlite3.connect(':memory:') as fixture:
        source.backup(fixture)
        mutations = [
            'DELETE FROM chunks WHERE id=(SELECT min(id) FROM chunks)',
            'UPDATE chunks SET vector=zeroblob(length(vector)) WHERE id=(SELECT min(id) FROM chunks)',
            'UPDATE chunks SET text=text || "corrupt" WHERE id=(SELECT min(id) FROM chunks)',
            'UPDATE chunks SET sha="stale" WHERE id=(SELECT min(id) FROM chunks)',
            'UPDATE files SET sha="stale" WHERE path=(SELECT min(path) FROM files)',
            'INSERT INTO chunks(path,start,end,sha,text,vector) SELECT path,start,end,sha,text,vector FROM chunks LIMIT 1',
        ]
        for mutation in mutations:
            fixture.execute('SAVEPOINT negative')
            fixture.execute(mutation)
            try:
                retrieve.check(fixture)
            except (RuntimeError, ValueError):
                pass
            else:
                raise AssertionError('Accepted corrupt/incomplete index: ' + mutation)
            finally:
                fixture.execute('ROLLBACK TO negative')
                fixture.execute('RELEASE negative')
        retrieve.check(fixture)
print('PASS: complete source/chunk/vector admission and 6 fail-closed negatives')
