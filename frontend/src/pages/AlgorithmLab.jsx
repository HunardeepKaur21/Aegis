import { useCallback, useEffect, useRef, useState } from 'react';
import {
  BarChart,
  Bar,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend,
  ResponsiveContainer,
} from 'recharts';
import {
  compareLabAlgorithms,
  getLabAlgorithms,
  runLabAlgorithm,
} from '../services/labService.js';

const CATEGORIES = [
  { id: 'Sorting', label: 'Sorting (9 Algos)', icon: '📊' },
  { id: 'Searching', label: 'Searching', icon: '🔍' },
  { id: 'Dynamic Programming', label: 'Dynamic Programming', icon: '⚡' },
  { id: 'Arrays', label: 'Arrays', icon: '🧱' },
  { id: 'Strings', label: 'Strings & KMP', icon: '🔤' },
  { id: 'Stacks', label: 'Stacks (LIFO)', icon: '📚' },
  { id: 'Queues', label: 'Queues (FIFO)', icon: '🚶' },
  { id: 'Trees', label: 'Trees', icon: '🌳' },
  { id: 'Heaps', label: 'Heaps', icon: '⛰️' },
  { id: 'Hashing', label: 'Hashing', icon: '🔑' },
  { id: 'Compare', label: 'Algorithm Comparison', icon: '⚖️' },
];

const SORTING_OPTIONS = [
  'Bubble Sort',
  'Selection Sort',
  'Insertion Sort',
  'Merge Sort',
  'Quick Sort',
  'Heap Sort',
  'Counting Sort',
  'Radix Sort',
  'Bucket Sort',
];

const SEARCHING_OPTIONS = [
  'Linear Search',
  'Binary Search',
  'Recursive Binary Search',
];

const DP_OPTIONS = [
  'Fibonacci (Memoization)',
  'Fibonacci (Tabulation)',
  '0/1 Knapsack',
  'Unbounded Knapsack',
  'Longest Common Subsequence',
  'Longest Increasing Subsequence',
  'Coin Change',
  'Matrix Chain Multiplication',
];

export default function AlgorithmLab() {
  const [activeCategory, setActiveCategory] = useState('Sorting');
  const [selectedAlgo, setSelectedAlgo] = useState('Quick Sort');
  const [inputArrayText, setInputArrayText] = useState('38, 27, 43, 3, 9, 82, 10, 19, 50, 15');
  const [searchTarget, setSearchTarget] = useState(43);
  const [stringText, setStringText] = useState('ABABDABACDABABCABAB');
  const [stringPattern, setStringPattern] = useState('ABABCABAB');
  const [dpParam, setDpParam] = useState(10);

  // Data structure specific states
  const [dsOperation, setDsOperation] = useState('insert');
  const [dsParamVal, setDsParamVal] = useState(99);
  const [dsIndexVal, setDsIndexVal] = useState(2);

  // Execution Result
  const [result, setResult] = useState(null);
  const [loading, setLoading] = useState(false);

  // Animation Playback State
  const [currentStepIdx, setCurrentStepIdx] = useState(0);
  const [isPlaying, setIsPlaying] = useState(false);
  const [speedMs, setSpeedMs] = useState(400); // delay in ms
  const playTimerRef = useRef(null);

  // Comparison State
  const [compAlgoA, setCompAlgoA] = useState('Quick Sort');
  const [compAlgoB, setCompAlgoB] = useState('Bubble Sort');
  const [comparisonResult, setComparisonResult] = useState(null);
  const [comparing, setComparing] = useState(false);

  // Parse custom array from string
  const parseInputArray = () => {
    return inputArrayText
      .split(',')
      .map((s) => parseInt(s.trim(), 10))
      .filter((n) => !isNaN(n));
  };

  const generateRandomArray = () => {
    const size = 10;
    const arr = [];
    for (let i = 0; i < size; ++i) {
      arr.push(Math.floor(Math.random() * 90) + 10);
    }
    setInputArrayText(arr.join(', '));
  };

  // Run selected algorithm
  const handleExecute = async () => {
    setLoading(true);
    setIsPlaying(false);
    clearInterval(playTimerRef.current);

    try {
      const arr = parseInputArray();
      const payload = {
        category: activeCategory,
        algorithm: selectedAlgo,
        input_data: arr,
        operation: dsOperation,
        param_val:
          activeCategory === 'Searching'
            ? Number(searchTarget)
            : activeCategory === 'Dynamic Programming'
            ? Number(dpParam)
            : activeCategory === 'Arrays'
            ? Number(dsIndexVal)
            : Number(dsParamVal),
        str_param1: stringText,
        str_param2: stringPattern,
      };

      const res = await runLabAlgorithm(payload);
      setResult(res);
      setCurrentStepIdx(0);
    } catch (err) {
      console.error(err);
    } finally {
      setLoading(false);
    }
  };

  // Execute comparison
  const handleCompare = async () => {
    setComparing(true);
    try {
      const arr = parseInputArray();
      const res = await compareLabAlgorithms({
        algorithm_a: compAlgoA,
        algorithm_b: compAlgoB,
        input_data: arr,
      });
      setComparisonResult(res);
    } catch (err) {
      console.error(err);
    } finally {
      setComparing(false);
    }
  };

  // Run on mount or when category switches
  useEffect(() => {
    if (activeCategory === 'Sorting') setSelectedAlgo('Quick Sort');
    else if (activeCategory === 'Searching') setSelectedAlgo('Binary Search');
    else if (activeCategory === 'Dynamic Programming') setSelectedAlgo('0/1 Knapsack');
    else if (activeCategory === 'Strings') setDsOperation('kmp');
    else if (activeCategory === 'Stacks') setDsOperation('push');
    else if (activeCategory === 'Queues') setDsOperation('enqueue');
    else if (activeCategory === 'Trees') setDsOperation('inorder');
    else if (activeCategory === 'Hashing') setDsOperation('chaining');
    else if (activeCategory === 'Heaps') setDsOperation('max_heap');

    if (activeCategory !== 'Compare') {
      handleExecute();
    }
  }, [activeCategory]);

  // Animation player step progression
  useEffect(() => {
    if (isPlaying && result?.steps && result.steps.length > 0) {
      playTimerRef.current = setInterval(() => {
        setCurrentStepIdx((prev) => {
          if (prev + 1 >= result.steps.length) {
            setIsPlaying(false);
            clearInterval(playTimerRef.current);
            return prev;
          }
          return prev + 1;
        });
      }, speedMs);
    } else {
      clearInterval(playTimerRef.current);
    }

    return () => clearInterval(playTimerRef.current);
  }, [isPlaying, speedMs, result?.steps?.length]);

  const stepForward = () => {
    if (result?.steps && currentStepIdx < result.steps.length - 1) {
      setCurrentStepIdx(currentStepIdx + 1);
    }
  };

  const stepBackward = () => {
    if (currentStepIdx > 0) {
      setCurrentStepIdx(currentStepIdx - 1);
    }
  };

  const resetAnimation = () => {
    setIsPlaying(false);
    clearInterval(playTimerRef.current);
    setCurrentStepIdx(0);
  };

  // Current step state snapshot
  const currentStep = result?.steps && result.steps[currentStepIdx] ? result.steps[currentStepIdx] : null;
  const currentArray = currentStep?.array_state || result?.result_array || parseInputArray();

  // Recharts data for comparison
  const compChartData = comparisonResult
    ? [
        {
          metric: 'Runtime (μs)',
          [compAlgoA]: Number(comparisonResult.result_a.runtime_us.toFixed(1)),
          [compAlgoB]: Number(comparisonResult.result_b.runtime_us.toFixed(1)),
        },
        {
          metric: 'Comparisons',
          [compAlgoA]: comparisonResult.result_a.comparisons,
          [compAlgoB]: comparisonResult.result_b.comparisons,
        },
        {
          metric: 'Swaps / Shifts',
          [compAlgoA]: comparisonResult.result_a.swaps,
          [compAlgoB]: comparisonResult.result_b.swaps,
        },
      ]
    : [];

  return (
    <div className="page-shell">
      {/* Header */}
      <div className="section-head">
        <div>
          <h1 className="page-title">Algorithm Laboratory</h1>
          <p className="page-subtitle">
            Run data structures and algorithms on your own input and explore their execution, complexity and performance.
          </p>
        </div>
        <div className="actions">
          <button className="btn btn-secondary" onClick={generateRandomArray}>
            🎲 Random Input
          </button>
          <button className="btn btn-primary" onClick={handleExecute} disabled={loading}>
            {loading ? 'Executing C++...' : '⚡ Execute in C++'}
          </button>
        </div>
      </div>

      {/* 12-Category Navigation Pills */}
      <div
        style={{
          display: 'flex',
          flexWrap: 'wrap',
          gap: '8px',
          paddingBottom: '14px',
          borderBottom: '1px solid var(--line)',
          marginBottom: '20px',
        }}
      >
        {CATEGORIES.map((cat) => (
          <button
            key={cat.id}
            className={`btn ${activeCategory === cat.id ? 'btn-primary' : 'btn-secondary'}`}
            style={{ fontSize: '12px', padding: '7px 12px', display: 'flex', alignItems: 'center', gap: '6px' }}
            onClick={() => setActiveCategory(cat.id)}
          >
            <span>{cat.icon}</span>
            <span>{cat.label}</span>
          </button>
        ))}
      </div>

      {/* =========================================================================
          SECTION 1: COMPARISON WORKBENCH (When activeCategory === 'Compare')
          ========================================================================= */}
      {activeCategory === 'Compare' ? (
        <div className="card">
          <div className="card-head">
            <div className="title">
              <span>⚖️ Algorithm Benchmark & Comparison Workbench</span>
            </div>
            <span className="tag">Dual C++ Execution</span>
          </div>

          <div
            style={{
              display: 'grid',
              gridTemplateColumns: 'repeat(auto-fit, minmax(240px, 1fr))',
              gap: '16px',
              background: 'var(--bg)',
              padding: '16px',
              borderRadius: '8px',
              border: '1px solid var(--line)',
              marginBottom: '20px',
            }}
          >
            <div>
              <label style={{ display: 'block', fontSize: '12px', color: 'var(--muted)', marginBottom: '6px' }}>
                Algorithm A:
              </label>
              <select
                value={compAlgoA}
                onChange={(e) => setCompAlgoA(e.target.value)}
                style={{
                  width: '100%',
                  background: 'var(--card)',
                  border: '1px solid var(--line)',
                  color: 'var(--text)',
                  padding: '8px 12px',
                  borderRadius: '6px',
                  fontSize: '13.5px',
                }}
              >
                {SORTING_OPTIONS.map((a) => (
                  <option key={'a-' + a} value={a}>
                    {a}
                  </option>
                ))}
              </select>
            </div>

            <div>
              <label style={{ display: 'block', fontSize: '12px', color: 'var(--muted)', marginBottom: '6px' }}>
                Algorithm B:
              </label>
              <select
                value={compAlgoB}
                onChange={(e) => setCompAlgoB(e.target.value)}
                style={{
                  width: '100%',
                  background: 'var(--card)',
                  border: '1px solid var(--line)',
                  color: 'var(--text)',
                  padding: '8px 12px',
                  borderRadius: '6px',
                  fontSize: '13.5px',
                }}
              >
                {SORTING_OPTIONS.map((b) => (
                  <option key={'b-' + b} value={b}>
                    {b}
                  </option>
                ))}
              </select>
            </div>

            <div style={{ display: 'flex', alignItems: 'flex-end' }}>
              <button
                className="btn btn-primary"
                style={{ width: '100%', padding: '10px' }}
                onClick={handleCompare}
                disabled={comparing}
              >
                {comparing ? 'Benchmarking...' : '⚡ Run Side-by-Side Benchmark'}
              </button>
            </div>
          </div>

          {comparisonResult && (
            <div>
              {/* Winner Banner */}
              <div className="winner-banner">
                <div className="winner-left">
                  <span className="winner-icon">🏆</span>
                  <div>
                    <div className="winner-title">Winner: {comparisonResult.winner}</div>
                    <div className="winner-desc">{comparisonResult.summary}</div>
                  </div>
                </div>
              </div>

              {/* Side by side comparison cards */}
              <div className="comparison-grid">
                <div className="comparison-card dp">
                  <div className="comparison-header">
                    <div className="comparison-title">{compAlgoA}</div>
                    <span className="comparison-badge optimal">
                      {comparisonResult.result_a.time_complexity}
                    </span>
                  </div>
                  <div className="comparison-metrics-row">
                    <div className="metric-box">
                      <div className="label">Runtime</div>
                      <div className="val" style={{ color: '#38BDF8' }}>
                        {comparisonResult.result_a.runtime_us.toFixed(1)}
                      </div>
                      <div className="sub">microseconds</div>
                    </div>
                    <div className="metric-box">
                      <div className="label">Comparisons</div>
                      <div className="val">{comparisonResult.result_a.comparisons}</div>
                      <div className="sub">operations</div>
                    </div>
                    <div className="metric-box">
                      <div className="label">Swaps</div>
                      <div className="val">{comparisonResult.result_a.swaps}</div>
                      <div className="sub">memory writes</div>
                    </div>
                  </div>
                  <div style={{ fontSize: '12px', color: 'var(--muted)' }}>
                    Space Complexity: <strong>{comparisonResult.result_a.space_complexity}</strong>
                  </div>
                </div>

                <div className="comparison-card greedy">
                  <div className="comparison-header">
                    <div className="comparison-title">{compAlgoB}</div>
                    <span className="comparison-badge heuristic">
                      {comparisonResult.result_b.time_complexity}
                    </span>
                  </div>
                  <div className="comparison-metrics-row">
                    <div className="metric-box">
                      <div className="label">Runtime</div>
                      <div className="val" style={{ color: '#FBBF24' }}>
                        {comparisonResult.result_b.runtime_us.toFixed(1)}
                      </div>
                      <div className="sub">microseconds</div>
                    </div>
                    <div className="metric-box">
                      <div className="label">Comparisons</div>
                      <div className="val">{comparisonResult.result_b.comparisons}</div>
                      <div className="sub">operations</div>
                    </div>
                    <div className="metric-box">
                      <div className="label">Swaps</div>
                      <div className="val">{comparisonResult.result_b.swaps}</div>
                      <div className="sub">memory writes</div>
                    </div>
                  </div>
                  <div style={{ fontSize: '12px', color: 'var(--muted)' }}>
                    Space Complexity: <strong>{comparisonResult.result_b.space_complexity}</strong>
                  </div>
                </div>
              </div>

              {/* Recharts Comparison Visualization */}
              <div
                style={{
                  marginTop: '20px',
                  background: 'var(--bg)',
                  padding: '16px',
                  borderRadius: '8px',
                  border: '1px solid var(--line)',
                }}
              >
                <div style={{ fontSize: '13px', fontWeight: 600, color: 'var(--text)', marginBottom: '12px' }}>
                  📊 Comparative Performance Chart
                </div>
                <div style={{ height: '240px', width: '100%' }}>
                  <ResponsiveContainer width="100%" height="100%">
                    <BarChart data={compChartData} margin={{ top: 10, right: 20, left: -10, bottom: 0 }}>
                      <CartesianGrid strokeDasharray="3 3" stroke="rgba(255,255,255,0.06)" />
                      <XAxis dataKey="metric" stroke="#64748B" fontSize={11.5} />
                      <YAxis stroke="#64748B" fontSize={11.5} />
                      <Tooltip
                        contentStyle={{
                          background: '#0F172A',
                          border: '1px solid #334155',
                          borderRadius: '6px',
                          fontSize: '12px',
                        }}
                        labelStyle={{ color: '#F8FAFC', fontWeight: 600 }}
                      />
                      <Legend wrapperStyle={{ fontSize: '12px', paddingTop: '6px' }} />
                      <Bar dataKey={compAlgoA} fill="#38BDF8" radius={[4, 4, 0, 0]} />
                      <Bar dataKey={compAlgoB} fill="#F59E0B" radius={[4, 4, 0, 0]} />
                    </BarChart>
                  </ResponsiveContainer>
                </div>
              </div>
            </div>
          )}
        </div>
      ) : (
        /* =========================================================================
            SECTION 2: ALGORITHM INTERACTIVE WORKSPACE
            ========================================================================= */
        <div>
          {/* Controls Panel */}
          <div className="card" style={{ marginBottom: '20px' }}>
            <div
              style={{
                display: 'grid',
                gridTemplateColumns: 'repeat(auto-fit, minmax(220px, 1fr))',
                gap: '16px',
                alignItems: 'flex-end',
              }}
            >
              {/* Algorithm or Operation Selector */}
              {activeCategory === 'Sorting' && (
                <div>
                  <label style={{ display: 'block', fontSize: '12px', color: 'var(--muted)', marginBottom: '6px' }}>
                    Algorithm:
                  </label>
                  <select
                    value={selectedAlgo}
                    onChange={(e) => setSelectedAlgo(e.target.value)}
                    style={{
                      width: '100%',
                      background: 'var(--bg)',
                      border: '1px solid var(--line)',
                      color: 'var(--text)',
                      padding: '8px 12px',
                      borderRadius: '6px',
                      fontSize: '13px',
                    }}
                  >
                    {SORTING_OPTIONS.map((a) => (
                      <option key={a} value={a}>
                        {a}
                      </option>
                    ))}
                  </select>
                </div>
              )}

              {activeCategory === 'Searching' && (
                <>
                  <div>
                    <label style={{ display: 'block', fontSize: '12px', color: 'var(--muted)', marginBottom: '6px' }}>
                      Search Algorithm:
                    </label>
                    <select
                      value={selectedAlgo}
                      onChange={(e) => setSelectedAlgo(e.target.value)}
                      style={{
                        width: '100%',
                        background: 'var(--bg)',
                        border: '1px solid var(--line)',
                        color: 'var(--text)',
                        padding: '8px 12px',
                        borderRadius: '6px',
                        fontSize: '13px',
                      }}
                    >
                      {SEARCHING_OPTIONS.map((a) => (
                        <option key={a} value={a}>
                          {a}
                        </option>
                      ))}
                    </select>
                  </div>
                  <div>
                    <label style={{ display: 'block', fontSize: '12px', color: 'var(--muted)', marginBottom: '6px' }}>
                      Target Value:
                    </label>
                    <input
                      type="number"
                      value={searchTarget}
                      onChange={(e) => setSearchTarget(Number(e.target.value))}
                      style={{
                        width: '100%',
                        background: 'var(--bg)',
                        border: '1px solid var(--line)',
                        color: 'var(--text)',
                        padding: '8px 12px',
                        borderRadius: '6px',
                        fontSize: '13px',
                      }}
                    />
                  </div>
                </>
              )}

              {activeCategory === 'Dynamic Programming' && (
                <>
                  <div>
                    <label style={{ display: 'block', fontSize: '12px', color: 'var(--muted)', marginBottom: '6px' }}>
                      DP Problem:
                    </label>
                    <select
                      value={selectedAlgo}
                      onChange={(e) => setSelectedAlgo(e.target.value)}
                      style={{
                        width: '100%',
                        background: 'var(--bg)',
                        border: '1px solid var(--line)',
                        color: 'var(--text)',
                        padding: '8px 12px',
                        borderRadius: '6px',
                        fontSize: '13px',
                      }}
                    >
                      {DP_OPTIONS.map((a) => (
                        <option key={a} value={a}>
                          {a}
                        </option>
                      ))}
                    </select>
                  </div>
                  <div>
                    <label style={{ display: 'block', fontSize: '12px', color: 'var(--muted)', marginBottom: '6px' }}>
                      Parameter (n / Capacity / Target):
                    </label>
                    <input
                      type="number"
                      min="1"
                      max="40"
                      value={dpParam}
                      onChange={(e) => setDpParam(Number(e.target.value))}
                      style={{
                        width: '100%',
                        background: 'var(--bg)',
                        border: '1px solid var(--line)',
                        color: 'var(--text)',
                        padding: '8px 12px',
                        borderRadius: '6px',
                        fontSize: '13px',
                      }}
                    />
                  </div>
                </>
              )}

              {activeCategory === 'Strings' && (
                <>
                  <div>
                    <label style={{ display: 'block', fontSize: '12px', color: 'var(--muted)', marginBottom: '6px' }}>
                      Operation:
                    </label>
                    <select
                      value={dsOperation}
                      onChange={(e) => setDsOperation(e.target.value)}
                      style={{
                        width: '100%',
                        background: 'var(--bg)',
                        border: '1px solid var(--line)',
                        color: 'var(--text)',
                        padding: '8px 12px',
                        borderRadius: '6px',
                        fontSize: '13px',
                      }}
                    >
                      <option value="kmp">KMP Pattern Matching</option>
                      <option value="palindrome">Palindrome Check</option>
                      <option value="anagram">Anagram Check</option>
                      <option value="frequency">Character Frequency</option>
                      <option value="reverse">String Reversal</option>
                    </select>
                  </div>
                  <div>
                    <label style={{ display: 'block', fontSize: '12px', color: 'var(--muted)', marginBottom: '6px' }}>
                      Text:
                    </label>
                    <input
                      type="text"
                      value={stringText}
                      onChange={(e) => setStringText(e.target.value)}
                      style={{
                        width: '100%',
                        background: 'var(--bg)',
                        border: '1px solid var(--line)',
                        color: 'var(--text)',
                        padding: '8px 12px',
                        borderRadius: '6px',
                        fontSize: '13px',
                      }}
                    />
                  </div>
                  {dsOperation === 'kmp' && (
                    <div>
                      <label style={{ display: 'block', fontSize: '12px', color: 'var(--muted)', marginBottom: '6px' }}>
                        Pattern to Find:
                      </label>
                      <input
                        type="text"
                        value={stringPattern}
                        onChange={(e) => setStringPattern(e.target.value)}
                        style={{
                          width: '100%',
                          background: 'var(--bg)',
                          border: '1px solid var(--line)',
                          color: 'var(--text)',
                          padding: '8px 12px',
                          borderRadius: '6px',
                          fontSize: '13px',
                        }}
                      />
                    </div>
                  )}
                </>
              )}

              {/* Input array for numerical categories */}
              {activeCategory !== 'Strings' && (
                <div style={{ flex: '1 1 300px' }}>
                  <label style={{ display: 'block', fontSize: '12px', color: 'var(--muted)', marginBottom: '6px' }}>
                    Input Array (comma separated):
                  </label>
                  <input
                    type="text"
                    value={inputArrayText}
                    onChange={(e) => setInputArrayText(e.target.value)}
                    style={{
                      width: '100%',
                      background: 'var(--bg)',
                      border: '1px solid var(--line)',
                      color: 'var(--text)',
                      padding: '8px 12px',
                      borderRadius: '6px',
                      fontSize: '13px',
                      fontFamily: 'monospace',
                    }}
                  />
                </div>
              )}
            </div>
          </div>

          {/* Execution Metrics & Complexity Summary */}
          {result && (
            <div className="kpi-grid" style={{ marginBottom: '20px' }}>
              <div className="kpi-card">
                <span className="label">Time Complexity</span>
                <span className="val" style={{ color: '#38BDF8', fontSize: '18px' }}>
                  {result.time_complexity}
                </span>
                <span className="sub">Theoretical Bound</span>
              </div>
              <div className="kpi-card">
                <span className="label">Space Complexity</span>
                <span className="val" style={{ color: '#4ADE80', fontSize: '18px' }}>
                  {result.space_complexity}
                </span>
                <span className="sub">Auxiliary Memory</span>
              </div>
              <div className="kpi-card">
                <span className="label">Execution Time</span>
                <span className="val" style={{ color: '#FBBF24', fontSize: '18px' }}>
                  {result.runtime_us?.toFixed(1)} <small style={{ fontSize: '12px' }}>μs</small>
                </span>
                <span className="sub">C++ Benchmark</span>
              </div>
              <div className="kpi-card">
                <span className="label">Operations</span>
                <span className="val" style={{ fontSize: '18px' }}>
                  {result.comparisons + result.swaps}
                </span>
                <span className="sub">
                  {result.comparisons} cmp / {result.swaps} swp
                </span>
              </div>
            </div>
          )}

          {/* Interactive Visualizer Canvas */}
          <div className="card">
            <div className="card-head">
              <div className="title">
                <span>🖥️ Execution Visualizer Workspace</span>
              </div>
              {result?.steps && result.steps.length > 0 && (
                <span className="tag">
                  Step {currentStepIdx + 1} of {result.steps.length}
                </span>
              )}
            </div>

            {/* Animation Playback Toolbar (If steps available) */}
            {result?.steps && result.steps.length > 0 && (
              <div
                style={{
                  display: 'flex',
                  flexWrap: 'wrap',
                  gap: '10px',
                  alignItems: 'center',
                  background: 'var(--bg)',
                  padding: '10px 14px',
                  borderRadius: '6px',
                  border: '1px solid var(--line)',
                  marginBottom: '16px',
                }}
              >
                <div style={{ display: 'flex', gap: '6px' }}>
                  <button
                    className="btn btn-secondary"
                    style={{ padding: '6px 12px' }}
                    onClick={stepBackward}
                    disabled={currentStepIdx === 0}
                  >
                    ⏮ Prev
                  </button>
                  <button
                    className={`btn ${isPlaying ? 'btn-secondary' : 'btn-primary'}`}
                    style={{ padding: '6px 16px' }}
                    onClick={() => setIsPlaying(!isPlaying)}
                  >
                    {isPlaying ? '⏸ Pause' : '▶ Play'}
                  </button>
                  <button
                    className="btn btn-secondary"
                    style={{ padding: '6px 12px' }}
                    onClick={stepForward}
                    disabled={currentStepIdx >= result.steps.length - 1}
                  >
                    Next ⏭
                  </button>
                  <button className="btn btn-secondary" style={{ padding: '6px 12px' }} onClick={resetAnimation}>
                    ↺ Reset
                  </button>
                </div>

                <div style={{ display: 'flex', alignItems: 'center', gap: '8px', marginLeft: 'auto' }}>
                  <span style={{ fontSize: '12px', color: 'var(--muted)' }}>Speed:</span>
                  <select
                    value={speedMs}
                    onChange={(e) => setSpeedMs(Number(e.target.value))}
                    style={{
                      background: 'var(--card)',
                      border: '1px solid var(--line)',
                      color: 'var(--text)',
                      padding: '4px 8px',
                      borderRadius: '4px',
                      fontSize: '12px',
                    }}
                  >
                    <option value={800}>0.5x (Slow)</option>
                    <option value={400}>1x (Normal)</option>
                    <option value={150}>2x (Fast)</option>
                  </select>
                </div>
              </div>
            )}

            {/* Step Description Trace */}
            {currentStep && (
              <div
                style={{
                  background: 'rgba(56, 189, 248, 0.08)',
                  borderLeft: '4px solid #38BDF8',
                  padding: '10px 14px',
                  borderRadius: '0 6px 6px 0',
                  fontSize: '13px',
                  color: 'var(--text)',
                  marginBottom: '16px',
                }}
              >
                <strong>Step {currentStep.step_number}:</strong> {currentStep.description}
              </div>
            )}

            {/* Visualizer Body based on category */}
            <div style={{ minHeight: '200px', display: 'flex', flexDirection: 'column', justifyContent: 'center' }}>
              {/* SORTING VISUALIZER (Bars) */}
              {activeCategory === 'Sorting' && (
                <div
                  style={{
                    display: 'flex',
                    alignItems: 'flex-end',
                    justifyContent: 'center',
                    gap: '8px',
                    height: '220px',
                    padding: '16px',
                    background: 'var(--bg)',
                    borderRadius: '8px',
                    border: '1px solid var(--line)',
                  }}
                >
                  {currentArray.map((val, idx) => {
                    const maxVal = Math.max(...currentArray, 10);
                    const heightPct = Math.max(12, Math.round((Math.abs(val) / maxVal) * 100));
                    const isPrimary = currentStep?.primary_index === idx;
                    const isSecondary = currentStep?.secondary_index === idx;
                    const isSwapped = currentStep?.is_swap && (isPrimary || isSecondary);
                    const isCompared = currentStep?.is_comparison && (isPrimary || isSecondary);

                    const barColor = isSwapped
                      ? '#EF4444' // red on swap
                      : isCompared
                      ? '#F59E0B' // amber on comparison
                      : isPrimary
                      ? '#38BDF8' // primary highlight
                      : 'rgba(56, 189, 248, 0.4)';

                    return (
                      <div
                        key={idx}
                        style={{
                          display: 'flex',
                          flexDirection: 'column',
                          alignItems: 'center',
                          flex: 1,
                          maxWidth: '45px',
                        }}
                      >
                        <span style={{ fontSize: '11px', color: 'var(--muted)', marginBottom: '4px' }}>
                          {val}
                        </span>
                        <div
                          style={{
                            width: '100%',
                            height: `${heightPct}%`,
                            background: barColor,
                            borderRadius: '4px 4px 0 0',
                            transition: 'height 0.2s ease, background 0.2s ease',
                          }}
                        />
                        <span style={{ fontSize: '10px', color: 'var(--muted)', marginTop: '4px' }}>
                          [{idx}]
                        </span>
                      </div>
                    );
                  })}
                </div>
              )}

              {/* SEARCHING VISUALIZER (Horizontal cells) */}
              {activeCategory === 'Searching' && (
                <div
                  style={{
                    display: 'flex',
                    flexWrap: 'wrap',
                    gap: '8px',
                    justifyContent: 'center',
                    padding: '24px',
                    background: 'var(--bg)',
                    borderRadius: '8px',
                    border: '1px solid var(--line)',
                  }}
                >
                  {currentArray.map((val, idx) => {
                    const isTarget = val === searchTarget;
                    const isCurrent = currentStep?.primary_index === idx;
                    const isHighlighted = currentStep?.highlighted_indices?.includes(idx);

                    return (
                      <div
                        key={idx}
                        style={{
                          background: isCurrent
                            ? 'rgba(56, 189, 248, 0.25)'
                            : isHighlighted
                            ? 'rgba(245, 158, 11, 0.2)'
                            : 'var(--card)',
                          border: `2px solid ${
                            isCurrent
                              ? '#38BDF8'
                              : isHighlighted
                              ? '#F59E0B'
                              : 'var(--line)'
                          }`,
                          borderRadius: '8px',
                          padding: '12px 18px',
                          textAlign: 'center',
                          minWidth: '60px',
                          boxShadow: isCurrent ? '0 0 12px rgba(56, 189, 248, 0.3)' : 'none',
                        }}
                      >
                        <div style={{ fontSize: '18px', fontWeight: 700, color: 'var(--text)' }}>{val}</div>
                        <div style={{ fontSize: '10.5px', color: 'var(--muted)', marginTop: '4px' }}>
                          Index {idx}
                        </div>
                        {isTarget && isCurrent && (
                          <div style={{ fontSize: '10px', color: 'var(--success)', fontWeight: 700, marginTop: '2px' }}>
                            MATCH!
                          </div>
                        )}
                      </div>
                    );
                  })}
                </div>
              )}

              {/* DYNAMIC PROGRAMMING MATRIX VISUALIZER */}
              {activeCategory === 'Dynamic Programming' && result?.matrix_data && (
                <div className="dp-table-container">
                  <table className="dp-matrix">
                    {result.col_headers?.length > 0 && (
                      <thead>
                        <tr>
                          <th>State \ Subproblem</th>
                          {result.col_headers.map((h, i) => (
                            <th key={i}>{h}</th>
                          ))}
                        </tr>
                      </thead>
                    )}
                    <tbody>
                      {result.matrix_data.map((row, rIdx) => (
                        <tr key={rIdx}>
                          <td className="row-header">
                            {result.row_headers?.[rIdx] || `Row #${rIdx}`}
                          </td>
                          {row.map((cell, cIdx) => (
                            <td key={cIdx} className={cell > 0 ? 'cell-highlight' : ''}>
                              {cell}
                            </td>
                          ))}
                        </tr>
                      ))}
                    </tbody>
                  </table>
                </div>
              )}

              {/* KEY-VALUE OR HASHING SLOTS VISUALIZER */}
              {result?.key_value_pairs && result.key_value_pairs.length > 0 && (
                <div
                  style={{
                    display: 'grid',
                    gridTemplateColumns: 'repeat(auto-fill, minmax(200px, 1fr))',
                    gap: '10px',
                    padding: '14px',
                    background: 'var(--bg)',
                    borderRadius: '8px',
                    border: '1px solid var(--line)',
                  }}
                >
                  {result.key_value_pairs.map((kv, i) => (
                    <div
                      key={i}
                      style={{
                        background: 'var(--card)',
                        border: '1px solid var(--line)',
                        borderRadius: '6px',
                        padding: '10px 12px',
                      }}
                    >
                      <div style={{ fontSize: '11.5px', color: 'var(--primary)', fontWeight: 600 }}>
                        {kv.key}
                      </div>
                      <div style={{ fontSize: '13px', color: 'var(--text)', marginTop: '4px', fontFamily: 'monospace' }}>
                        {kv.value}
                      </div>
                    </div>
                  ))}
                </div>
              )}

              {/* RESULT TEXT / MESSAGE */}
              {result?.message && (
                <div
                  style={{
                    marginTop: '16px',
                    padding: '12px 16px',
                    background: 'var(--bg)',
                    border: '1px solid var(--line)',
                    borderRadius: '6px',
                    fontSize: '13px',
                    color: 'var(--text)',
                  }}
                >
                  <strong>Result Output:</strong> {result.message}
                </div>
              )}
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
