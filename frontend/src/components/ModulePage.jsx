// Placeholder shell for pages whose backend module is not built yet.
export default function ModulePage({ title, summary, owner, algorithms, endpoints }) {
  return (
    <>
      <header className="page-head">
        <h1>{title}</h1>
        <p>{summary}</p>
      </header>
      <section className="card pending">
        <h2>This page connects in a later step</h2>
        <p>The layout is ready. Its C++ algorithms and REST endpoints are built and tested first, then this page is wired to them.</p>
        <dl>
          <dt>Team module</dt><dd>{owner}</dd>
          <dt>C++ algorithms</dt><dd>{algorithms.join(', ')}</dd>
          <dt>Planned endpoints</dt>
          <dd>{endpoints.map((e) => <code key={e}>{e}</code>)}</dd>
        </dl>
      </section>
    </>
  );
}
