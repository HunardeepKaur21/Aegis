import { useCallback, useEffect, useState } from 'react';
import {
  BarChart,
  Bar,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend,
  ResponsiveContainer,
  Cell
} from 'recharts';
import {
  allocateResource,
  createResource,
  deleteResource,
  getAllocations,
  getResourceOverview,
  getResources,
  getShelters,
  optimizeResources,
  updateResource,
} from '../services/resourceService.js';

const CATEGORIES = [
  'All',
  'Food',
  'Drinking Water',
  'Medicines',
  'First Aid',
  'Rescue Equipment',
  'Blankets',
  'Other',
];

const PRESET_CAPACITIES = [
  { label: 'Light Drone / ATV (25 kg)', value: 25 },
  { label: 'Tactical Pickup (100 kg)', value: 100 },
  { label: 'Supply Van (250 kg)', value: 250 },
  { label: 'Heavy Cargo Truck (500 kg)', value: 500 },
];

export default function ResourceOptimizer() {
  // Main Data States
  const [resources, setResources] = useState([]);
  const [allocations, setAllocations] = useState([]);
  const [shelters, setShelters] = useState([]);
  const [overview, setOverview] = useState({
    total_items: 0,
    total_units: 0,
    total_weight_kg: 0,
    total_priority_value: 0,
    total_allocations: 0,
  });

  // Filter & Search
  const [search, setSearch] = useState('');
  const [selectedCategory, setSelectedCategory] = useState('All');
  const [sortBy, setSortBy] = useState('ratio');
  const [sortAsc, setSortAsc] = useState(false);

  // Optimizer Workbench State
  const [capacity, setCapacity] = useState(100);
  const [optimizerShelter, setOptimizerShelter] = useState('0');
  const [selectedCandidateIds, setSelectedCandidateIds] = useState(new Set());
  const [optimizing, setOptimizing] = useState(false);
  const [optimizationResult, setOptimizationResult] = useState(null);
  const [activeTab, setActiveTab] = useState('loadout'); // 'loadout' | 'matrix'

  // Modals
  const [showAddModal, setShowAddModal] = useState(false);
  const [showEditModal, setShowEditModal] = useState(false);
  const [showAllocateModal, setShowAllocateModal] = useState(false);
  const [activeItem, setActiveItem] = useState(null);

  // Form States
  const [itemForm, setItemForm] = useState({
    name: '',
    category: 'Food',
    available_quantity: 50,
    unit_weight: 10,
    value_score: 80,
    shelter_id: '',
  });

  const [allocForm, setAllocForm] = useState({
    quantity: 1,
    shelter_id: '',
    request_id: '',
  });

  // UI Toast
  const [toast, setToast] = useState(null);
  const [loading, setLoading] = useState(false);

  const showNotification = (text, type = 'success') => {
    setToast({ text, type });
    setTimeout(() => setToast(null), 3000);
  };

  // Load all initial data
  const loadData = useCallback(async () => {
    setLoading(true);
    try {
      const [resList, allocList, ovData, shelterList] = await Promise.all([
        getResources(),
        getAllocations(),
        getResourceOverview(),
        getShelters(),
      ]);
      setResources(resList || []);
      setAllocations(allocList || []);
      setOverview(ovData || {});
      setShelters(shelterList || []);

      // Auto-select all candidate IDs for the optimizer
      if (resList && resList.length > 0) {
        setSelectedCandidateIds(new Set(resList.map((r) => r.id)));
      }
    } catch (err) {
      showNotification('Failed to load resource data: ' + err.message, 'error');
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    loadData();
  }, [loadData]);

  // Run Knapsack Optimization
  const handleOptimize = async (silent = false) => {
    if (capacity <= 0) {
      if (!silent) showNotification('Please enter a valid payload capacity (> 0 kg)', 'error');
      return;
    }
    setOptimizing(true);
    try {
      const payload = {
        capacity_limit: Number(capacity),
        shelter_id: optimizerShelter !== '0' ? Number(optimizerShelter) : undefined,
        item_ids: Array.from(selectedCandidateIds),
      };
      const res = await optimizeResources(payload);
      setOptimizationResult(res);
      if (!silent) {
        showNotification('Supply optimization computed successfully!', 'success');
      }
    } catch (err) {
      if (!silent) {
        showNotification('Optimization error: ' + err.message, 'error');
      }
    } finally {
      setOptimizing(false);
    }
  };

  // Run initial optimization silently once data arrives
  useEffect(() => {
    if (resources.length > 0 && !optimizationResult && !optimizing) {
      handleOptimize(true);
    }
  }, [resources.length]);

  // Handle Add Resource
  const handleCreateResource = async (e) => {
    e.preventDefault();
    if (!itemForm.name.trim()) {
      showNotification('Please provide a resource name.', 'error');
      return;
    }
    try {
      await createResource({
        name: itemForm.name.trim(),
        category: itemForm.category,
        available_quantity: Number(itemForm.available_quantity),
        unit_weight: Number(itemForm.unit_weight),
        value_score: Number(itemForm.value_score),
        shelter_id: itemForm.shelter_id ? Number(itemForm.shelter_id) : undefined,
      });
      showNotification(`Resource "${itemForm.name}" added to inventory.`);
      setShowAddModal(false);
      setItemForm({
        name: '',
        category: 'Food',
        available_quantity: 50,
        unit_weight: 10,
        value_score: 80,
        shelter_id: '',
      });
      loadData();
    } catch (err) {
      showNotification('Error creating resource: ' + err.message, 'error');
    }
  };

  // Handle Edit Resource
  const handleUpdateResource = async (e) => {
    e.preventDefault();
    if (!activeItem) return;
    try {
      await updateResource(activeItem.id, {
        name: itemForm.name.trim(),
        category: itemForm.category,
        available_quantity: Number(itemForm.available_quantity),
        unit_weight: Number(itemForm.unit_weight),
        value_score: Number(itemForm.value_score),
        shelter_id: itemForm.shelter_id ? Number(itemForm.shelter_id) : undefined,
      });
      showNotification(`Resource #${activeItem.id} updated.`);
      setShowEditModal(false);
      loadData();
    } catch (err) {
      showNotification('Error updating resource: ' + err.message, 'error');
    }
  };

  // Handle Delete Resource
  const handleDeleteResource = async (item) => {
    if (!window.confirm(`Are you sure you want to delete "${item.name}" from inventory?`)) {
      return;
    }
    try {
      await deleteResource(item.id);
      showNotification(`Resource "${item.name}" removed from inventory.`);
      loadData();
    } catch (err) {
      showNotification('Error deleting resource: ' + err.message, 'error');
    }
  };

  // Handle Allocate
  const handleAllocate = async (e) => {
    e.preventDefault();
    if (!activeItem) return;
    const qty = Number(allocForm.quantity);
    if (qty <= 0) {
      showNotification('Allocated quantity must be greater than 0', 'error');
      return;
    }
    if (qty > activeItem.available_quantity) {
      showNotification(`Cannot allocate ${qty} units. Only ${activeItem.available_quantity} available.`, 'error');
      return;
    }
    try {
      await allocateResource({
        resource_id: activeItem.id,
        quantity: qty,
        shelter_id: allocForm.shelter_id ? Number(allocForm.shelter_id) : undefined,
        request_id: allocForm.request_id ? Number(allocForm.request_id) : undefined,
      });
      showNotification(`Dispatched ${qty} units of "${activeItem.name}".`);
      setShowAllocateModal(false);
      setAllocForm({ quantity: 1, shelter_id: '', request_id: '' });
      loadData();
    } catch (err) {
      showNotification('Allocation error: ' + err.message, 'error');
    }
  };

  const openEditModal = (item) => {
    setActiveItem(item);
    setItemForm({
      name: item.name,
      category: item.category,
      available_quantity: item.available_quantity,
      unit_weight: item.unit_weight,
      value_score: item.value_score,
      shelter_id: item.shelter_id ? String(item.shelter_id) : '',
    });
    setShowEditModal(true);
  };

  const openAllocateModal = (item) => {
    setActiveItem(item);
    setAllocForm({
      quantity: 1,
      shelter_id: item.shelter_id ? String(item.shelter_id) : (shelters[0]?.id ? String(shelters[0].id) : ''),
      request_id: '',
    });
    setShowAllocateModal(true);
  };

  // Candidate selection toggles
  const toggleCandidate = (id) => {
    const next = new Set(selectedCandidateIds);
    if (next.has(id)) {
      next.delete(id);
    } else {
      next.add(id);
    }
    setSelectedCandidateIds(next);
  };

  const selectAllCandidates = () => {
    setSelectedCandidateIds(new Set(resources.map((r) => r.id)));
  };

  const clearAllCandidates = () => {
    setSelectedCandidateIds(new Set());
  };

  // Filtered and sorted resources list
  const filteredResources = resources.filter((item) => {
    const matchCat = selectedCategory === 'All' || item.category.toLowerCase() === selectedCategory.toLowerCase();
    const matchSearch =
      !search.trim() ||
      item.name.toLowerCase().includes(search.toLowerCase()) ||
      item.category.toLowerCase().includes(search.toLowerCase());
    return matchCat && matchSearch;
  });

  filteredResources.sort((a, b) => {
    let diff = 0;
    if (sortBy === 'name') diff = a.name.localeCompare(b.name);
    else if (sortBy === 'weight') diff = a.unit_weight - b.unit_weight;
    else if (sortBy === 'value') diff = a.value_score - b.value_score;
    else if (sortBy === 'quantity') diff = a.available_quantity - b.available_quantity;
    else diff = a.ratio - b.ratio; // default ratio
    return sortAsc ? diff : -diff;
  });

  // Recharts data preparation for Algorithm Comparison
  const chartData = optimizationResult
    ? [
        {
          metric: 'Priority Value',
          DP: optimizationResult.dp_result.total_value,
          Greedy: optimizationResult.greedy_result.total_value,
          unit: 'pts',
        },
        {
          metric: 'Payload Loaded (kg)',
          DP: Number(optimizationResult.dp_result.total_weight.toFixed(1)),
          Greedy: Number(optimizationResult.greedy_result.total_weight.toFixed(1)),
          unit: 'kg',
        },
        {
          metric: 'Remaining Cap. (kg)',
          DP: Number(optimizationResult.dp_result.remaining_capacity.toFixed(1)),
          Greedy: Number(optimizationResult.greedy_result.remaining_capacity.toFixed(1)),
          unit: 'kg',
        },
        {
          metric: 'Runtime (μs)',
          DP: Number(optimizationResult.dp_result.runtime_us.toFixed(0)),
          Greedy: Number(optimizationResult.greedy_result.runtime_us.toFixed(0)),
          unit: 'μs',
        },
      ]
    : [];

  const getCategoryClass = (cat = '') => {
    return cat.toLowerCase().replace(/\s+/g, '-');
  };

  return (
    <div className="page-shell">
      {/* Toast Notification Banner */}
      {toast && (
        <div className={`toast-banner ${toast.type}`}>
          <span>{toast.text}</span>
          <button className="toast-close" onClick={() => setToast(null)}>
            ×
          </button>
        </div>
      )}

      {/* Page Header */}
      <div className="section-head">
        <div>
          <h1 className="page-title">Resource Optimizer</h1>
          <p className="page-subtitle">
            Optimize emergency supplies, maximize resource value and distribute limited supplies efficiently.
          </p>
        </div>
        <div className="actions" style={{ display: 'flex', gap: '10px' }}>
          <button className="btn btn-secondary" onClick={() => setShowAddModal(true)}>
            + Add Supply Item
          </button>
          <button className="btn btn-primary" onClick={() => handleOptimize(false)} disabled={optimizing}>
            {optimizing ? 'Optimizing...' : '⚡ Recompute Loadout'}
          </button>
        </div>
      </div>

      {/* KPI Overview Metrics */}
      <div className="kpi-grid">
        <div className="kpi-card">
          <span className="label">Resource Types</span>
          <span className="val">{overview.total_items || 0}</span>
          <span className="sub">Item Categories</span>
        </div>
        <div className="kpi-card">
          <span className="label">Available Units</span>
          <span className="val">{overview.total_units?.toLocaleString() || 0}</span>
          <span className="sub">Ready for Dispatch</span>
        </div>
        <div className="kpi-card">
          <span className="label">Inventory Mass</span>
          <span className="val">
            {overview.total_weight_kg ? Number(overview.total_weight_kg).toLocaleString(undefined, { maximumFractionDigits: 1 }) : 0}{' '}
            <small style={{ fontSize: '13px', color: 'var(--muted)', fontWeight: 400 }}>kg</small>
          </span>
          <span className="sub">Total Cargo Weight</span>
        </div>
        <div className="kpi-card">
          <span className="label">Total Priority Points</span>
          <span className="val" style={{ color: 'var(--primary)' }}>
            {overview.total_priority_value?.toLocaleString() || 0}
          </span>
          <span className="sub">Cumulative Score</span>
        </div>
      </div>

      {/* =========================================================================
          SECTION 1: KNAPSACK OPTIMIZATION WORKBENCH & COMPARISON
          ========================================================================= */}
      <div className="card" style={{ marginBottom: '24px' }}>
        <div className="card-head">
          <div className="title">
            <span>⚡ Knapsack Optimization Workbench</span>
          </div>
          <span className="tag">0/1 DP vs Greedy Heuristic</span>
        </div>

        {/* Vehicle / Capacity Controls */}
        <div
          style={{
            display: 'grid',
            gridTemplateColumns: 'repeat(auto-fit, minmax(220px, 1fr))',
            gap: '16px',
            background: 'var(--bg)',
            padding: '16px',
            borderRadius: '8px',
            border: '1px solid var(--line)',
            marginBottom: '16px',
          }}
        >
          <div>
            <label style={{ display: 'block', fontSize: '12px', color: 'var(--muted)', marginBottom: '6px' }}>
              Payload Capacity Limit (kg):
            </label>
            <input
              type="number"
              min="1"
              max="10000"
              value={capacity}
              onChange={(e) => setCapacity(Number(e.target.value))}
              style={{
                width: '100%',
                background: 'var(--card)',
                border: '1px solid var(--line)',
                color: 'var(--text)',
                padding: '8px 12px',
                borderRadius: '6px',
                fontSize: '15px',
                fontWeight: 600,
              }}
            />
          </div>

          <div>
            <label style={{ display: 'block', fontSize: '12px', color: 'var(--muted)', marginBottom: '6px' }}>
              Quick Presets:
            </label>
            <div style={{ display: 'flex', flexWrap: 'wrap', gap: '6px' }}>
              {PRESET_CAPACITIES.map((p) => (
                <button
                  key={p.value}
                  className={`btn ${capacity === p.value ? 'btn-primary' : 'btn-secondary'}`}
                  style={{ fontSize: '11.5px', padding: '6px 10px' }}
                  onClick={() => setCapacity(p.value)}
                >
                  {p.label}
                </button>
              ))}
            </div>
          </div>

          <div>
            <label style={{ display: 'block', fontSize: '12px', color: 'var(--muted)', marginBottom: '6px' }}>
              Shelter Filter:
            </label>
            <select
              value={optimizerShelter}
              onChange={(e) => setOptimizerShelter(e.target.value)}
              style={{
                width: '100%',
                background: 'var(--card)',
                border: '1px solid var(--line)',
                color: 'var(--text)',
                padding: '8px 12px',
                borderRadius: '6px',
                fontSize: '13px',
              }}
            >
              <option value="0">All Shelters / Central Stock</option>
              {shelters.map((s) => (
                <option key={s.id} value={s.id}>
                  {s.name}
                </option>
              ))}
            </select>
          </div>
        </div>

        {/* Selected Candidates Bar */}
        <div
          style={{
            display: 'flex',
            justifyContent: 'space-between',
            alignItems: 'center',
            fontSize: '12.5px',
            color: 'var(--muted)',
            marginBottom: '16px',
          }}
        >
          <div>
            Included Candidates:{' '}
            <strong style={{ color: 'var(--text)' }}>
              {selectedCandidateIds.size} of {resources.length} items
            </strong>
          </div>
          <div style={{ display: 'flex', gap: '10px' }}>
            <button
              onClick={selectAllCandidates}
              style={{
                background: 'transparent',
                border: 0,
                color: 'var(--primary)',
                cursor: 'pointer',
                fontSize: '12px',
                textDecoration: 'underline',
              }}
            >
              Select All
            </button>
            <button
              onClick={clearAllCandidates}
              style={{
                background: 'transparent',
                border: 0,
                color: 'var(--muted)',
                cursor: 'pointer',
                fontSize: '12px',
                textDecoration: 'underline',
              }}
            >
              Clear
            </button>
          </div>
        </div>

        {/* Winner Announcement Banner */}
        {optimizationResult && (
          <div className="winner-banner">
            <div className="winner-left">
              <span className="winner-icon">🏆</span>
              <div>
                <div className="winner-title">{optimizationResult.comparison.winner}</div>
                <div className="winner-desc">{optimizationResult.comparison.analysis_summary}</div>
              </div>
            </div>
            <div style={{ textAlign: 'right', whiteSpace: 'nowrap' }}>
              <span style={{ fontSize: '11px', color: 'var(--muted)', textTransform: 'uppercase' }}>
                Value Delta
              </span>
              <div
                style={{
                  fontSize: '18px',
                  fontWeight: 700,
                  color: optimizationResult.comparison.value_difference >= 0 ? 'var(--success)' : 'var(--warning)',
                }}
              >
                {optimizationResult.comparison.value_difference >= 0 ? '+' : ''}
                {optimizationResult.comparison.value_difference} pts
              </div>
            </div>
          </div>
        )}

        {/* Dual Performance Cards */}
        {optimizationResult && (
          <div className="comparison-grid">
            {/* Dynamic Programming Card */}
            <div className="comparison-card dp">
              <div className="comparison-header">
                <div className="comparison-title">
                  <span>0/1 Knapsack (DP)</span>
                </div>
                <span className="comparison-badge optimal">Globally Optimal</span>
              </div>
              <div className="comparison-metrics-row">
                <div className="metric-box">
                  <div className="label">Total Value</div>
                  <div className="val" style={{ color: '#38BDF8' }}>
                    {optimizationResult.dp_result.total_value}
                  </div>
                  <div className="sub">Priority pts</div>
                </div>
                <div className="metric-box">
                  <div className="label">Payload Loaded</div>
                  <div className="val">{optimizationResult.dp_result.total_weight.toFixed(1)}</div>
                  <div className="sub">of {capacity} kg</div>
                </div>
                <div className="metric-box">
                  <div className="label">Items Packed</div>
                  <div className="val">{optimizationResult.dp_result.selected_items.length}</div>
                  <div className="sub">
                    {((optimizationResult.dp_result.total_weight / capacity) * 100).toFixed(0)}% util
                  </div>
                </div>
              </div>
              <div style={{ fontSize: '11.5px', color: 'var(--muted)', display: 'flex', justifyContent: 'space-between' }}>
                <span>Remaining: {optimizationResult.dp_result.remaining_capacity.toFixed(1)} kg</span>
                <span>Runtime: {optimizationResult.dp_result.runtime_us.toFixed(1)} μs</span>
              </div>
            </div>

            {/* Greedy Heuristic Card */}
            <div className="comparison-card greedy">
              <div className="comparison-header">
                <div className="comparison-title">
                  <span>Greedy Heuristic</span>
                </div>
                <span className="comparison-badge heuristic">Value-to-Weight Ratio</span>
              </div>
              <div className="comparison-metrics-row">
                <div className="metric-box">
                  <div className="label">Total Value</div>
                  <div className="val" style={{ color: '#FBBF24' }}>
                    {optimizationResult.greedy_result.total_value}
                  </div>
                  <div className="sub">Priority pts</div>
                </div>
                <div className="metric-box">
                  <div className="label">Payload Loaded</div>
                  <div className="val">{optimizationResult.greedy_result.total_weight.toFixed(1)}</div>
                  <div className="sub">of {capacity} kg</div>
                </div>
                <div className="metric-box">
                  <div className="label">Items Packed</div>
                  <div className="val">{optimizationResult.greedy_result.selected_items.length}</div>
                  <div className="sub">
                    {((optimizationResult.greedy_result.total_weight / capacity) * 100).toFixed(0)}% util
                  </div>
                </div>
              </div>
              <div style={{ fontSize: '11.5px', color: 'var(--muted)', display: 'flex', justifyContent: 'space-between' }}>
                <span>Remaining: {optimizationResult.greedy_result.remaining_capacity.toFixed(1)} kg</span>
                <span>Runtime: {optimizationResult.greedy_result.runtime_us.toFixed(1)} μs</span>
              </div>
            </div>
          </div>
        )}

        {/* Recharts Algorithm Comparison Visualization */}
        {optimizationResult && chartData.length > 0 && (
          <div style={{ marginTop: '20px', background: 'var(--bg)', padding: '16px', borderRadius: '8px', border: '1px solid var(--line)' }}>
            <div style={{ fontSize: '13px', fontWeight: 600, color: 'var(--text)', marginBottom: '12px' }}>
              📊 Algorithm Efficiency & Metric Comparison (DP vs Greedy)
            </div>
            <div style={{ height: '240px', width: '100%' }}>
              <ResponsiveContainer width="100%" height="100%">
                <BarChart data={chartData} margin={{ top: 10, right: 20, left: -10, bottom: 0 }}>
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
                  <Bar dataKey="DP" fill="#38BDF8" name="Dynamic Programming (0/1)" radius={[4, 4, 0, 0]} />
                  <Bar dataKey="Greedy" fill="#F59E0B" name="Greedy (Ratio)" radius={[4, 4, 0, 0]} />
                </BarChart>
              </ResponsiveContainer>
            </div>
          </div>
        )}

        {/* Tab Switcher for Loadout vs DP Matrix */}
        {optimizationResult && (
          <div style={{ marginTop: '24px' }}>
            <div style={{ display: 'flex', gap: '8px', borderBottom: '1px solid var(--line)', paddingBottom: '8px' }}>
              <button
                className={`btn ${activeTab === 'loadout' ? 'btn-primary' : 'btn-secondary'}`}
                style={{ fontSize: '12.5px' }}
                onClick={() => setActiveTab('loadout')}
              >
                🎒 Optimized Manifest Loadout
              </button>
              <button
                className={`btn ${activeTab === 'matrix' ? 'btn-primary' : 'btn-secondary'}`}
                style={{ fontSize: '12.5px' }}
                onClick={() => setActiveTab('matrix')}
              >
                🔬 Interactive DP Matrix Table & Decision Log
              </button>
            </div>

            {/* TAB 1: Loadout Breakdown */}
            {activeTab === 'loadout' && (
              <div style={{ marginTop: '16px' }}>
                <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(320px, 1fr))', gap: '16px' }}>
                  {/* DP Selected Items */}
                  <div>
                    <h4 style={{ fontSize: '13.5px', color: '#38BDF8', marginBottom: '8px' }}>
                      DP Optimal Manifest ({optimizationResult.dp_result.selected_items.length} items)
                    </h4>
                    <div style={{ display: 'flex', flexDirection: 'column', gap: '8px' }}>
                      {optimizationResult.dp_result.selected_items.map((item) => (
                        <div
                          key={'dp-' + item.id}
                          style={{
                            background: 'var(--bg)',
                            border: '1px solid var(--line)',
                            borderRadius: '6px',
                            padding: '10px 12px',
                            display: 'flex',
                            justifyContent: 'space-between',
                            alignItems: 'center',
                          }}
                        >
                          <div>
                            <div style={{ fontWeight: 600, color: 'var(--text)', fontSize: '13px' }}>{item.name}</div>
                            <div style={{ fontSize: '11.5px', color: 'var(--muted)', marginTop: '2px' }}>
                              <span className={`category-badge ${getCategoryClass(item.category)}`}>
                                {item.category}
                              </span>{' '}
                              • {item.unit_weight} kg • Ratio: {item.ratio.toFixed(1)}
                            </div>
                          </div>
                          <div style={{ textAlign: 'right' }}>
                            <div style={{ color: 'var(--primary)', fontWeight: 700, fontSize: '14px' }}>
                              +{item.value_score} pts
                            </div>
                            <button
                              className="btn btn-secondary"
                              style={{ padding: '2px 8px', fontSize: '11px', marginTop: '4px' }}
                              onClick={() => openAllocateModal(item)}
                            >
                              Dispatch
                            </button>
                          </div>
                        </div>
                      ))}
                    </div>
                  </div>

                  {/* Greedy Selected Items */}
                  <div>
                    <h4 style={{ fontSize: '13.5px', color: '#FBBF24', marginBottom: '8px' }}>
                      Greedy Selected Manifest ({optimizationResult.greedy_result.selected_items.length} items)
                    </h4>
                    <div style={{ display: 'flex', flexDirection: 'column', gap: '8px' }}>
                      {optimizationResult.greedy_result.selected_items.map((item) => (
                        <div
                          key={'gr-' + item.id}
                          style={{
                            background: 'var(--bg)',
                            border: '1px solid var(--line)',
                            borderRadius: '6px',
                            padding: '10px 12px',
                            display: 'flex',
                            justifyContent: 'space-between',
                            alignItems: 'center',
                          }}
                        >
                          <div>
                            <div style={{ fontWeight: 600, color: 'var(--text)', fontSize: '13px' }}>{item.name}</div>
                            <div style={{ fontSize: '11.5px', color: 'var(--muted)', marginTop: '2px' }}>
                              <span className={`category-badge ${getCategoryClass(item.category)}`}>
                                {item.category}
                              </span>{' '}
                              • {item.unit_weight} kg • Ratio: {item.ratio.toFixed(1)}
                            </div>
                          </div>
                          <div style={{ textAlign: 'right' }}>
                            <div style={{ color: '#FBBF24', fontWeight: 700, fontSize: '14px' }}>
                              +{item.value_score} pts
                            </div>
                            <button
                              className="btn btn-secondary"
                              style={{ padding: '2px 8px', fontSize: '11px', marginTop: '4px' }}
                              onClick={() => openAllocateModal(item)}
                            >
                              Dispatch
                            </button>
                          </div>
                        </div>
                      ))}
                    </div>
                  </div>
                </div>
              </div>
            )}

            {/* TAB 2: Interactive DP Matrix Table */}
            {activeTab === 'matrix' && (
              <div style={{ marginTop: '16px' }}>
                <p style={{ fontSize: '12.5px', color: 'var(--muted)', marginBottom: '10px' }}>
                  The 2D dynamic programming matrix represents subproblem recurrence:{' '}
                  <code>dp[i][w] = max(dp[i-1][w], dp[i-1][w - w_i] + v_i)</code>. Columns represent discrete payload capacity steps, and rows represent candidate items considered.
                </p>

                {/* DP Table */}
                <div className="dp-table-container">
                  <table className="dp-matrix">
                    <thead>
                      <tr>
                        <th style={{ textAlign: 'left', minWidth: '180px' }}>Item \ Capacity (kg)</th>
                        {optimizationResult.dp_result.capacity_headers.map((cap, idx) => (
                          <th key={'cap-' + idx}>{cap} kg</th>
                        ))}
                      </tr>
                    </thead>
                    <tbody>
                      {optimizationResult.dp_result.dp_table.map((row, rIdx) => (
                        <tr key={'row-' + rIdx}>
                          <td className="row-header">
                            {optimizationResult.dp_result.item_headers[rIdx] || `Item #${rIdx}`}
                          </td>
                          {row.map((val, cIdx) => (
                            <td
                              key={'cell-' + rIdx + '-' + cIdx}
                              className={val > 0 && rIdx === optimizationResult.dp_result.dp_table.length - 1 ? 'cell-highlight' : ''}
                            >
                              {val}
                            </td>
                          ))}
                        </tr>
                      ))}
                    </tbody>
                  </table>
                </div>

                {/* Step-by-Step Backtracking Explanations */}
                <div style={{ marginTop: '16px' }}>
                  <h4 style={{ fontSize: '13px', color: 'var(--text)', marginBottom: '8px' }}>
                    📝 Backtracking Decision Trace:
                  </h4>
                  <div className="decisions-log">
                    {optimizationResult.dp_result.explanations.map((exp, i) => (
                      <div
                        key={'exp-' + i}
                        className={`decision-item ${exp.includes('selected') ? 'selected' : 'excluded'}`}
                      >
                        {exp}
                      </div>
                    ))}
                  </div>
                </div>
              </div>
            )}
          </div>
        )}
      </div>

      {/* =========================================================================
          SECTION 2: RESOURCE INVENTORY MANAGEMENT
          ========================================================================= */}
      <div className="card" style={{ marginBottom: '24px' }}>
        <div className="card-head">
          <div className="title">
            <span>📦 Emergency Supply Inventory</span>
          </div>
          <span className="tag">{filteredResources.length} Items</span>
        </div>

        {/* Toolbar: Search, Category Pills, Sort */}
        <div
          style={{
            display: 'flex',
            flexWrap: 'wrap',
            gap: '12px',
            alignItems: 'center',
            justifyContent: 'space-between',
            marginBottom: '16px',
          }}
        >
          {/* Search Box */}
          <div style={{ flex: '1 1 240px', maxWidth: '320px' }}>
            <input
              type="text"
              placeholder="🔍 Search supplies or category..."
              value={search}
              onChange={(e) => setSearch(e.target.value)}
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

          {/* Category Pills */}
          <div style={{ display: 'flex', flexWrap: 'wrap', gap: '6px' }}>
            {CATEGORIES.map((cat) => (
              <button
                key={cat}
                className={`btn ${selectedCategory === cat ? 'btn-primary' : 'btn-secondary'}`}
                style={{ fontSize: '11.5px', padding: '5px 10px' }}
                onClick={() => setSelectedCategory(cat)}
              >
                {cat}
              </button>
            ))}
          </div>

          {/* Sort Selector */}
          <div style={{ display: 'flex', alignItems: 'center', gap: '6px' }}>
            <span style={{ fontSize: '12px', color: 'var(--muted)' }}>Sort:</span>
            <select
              value={sortBy}
              onChange={(e) => setSortBy(e.target.value)}
              style={{
                background: 'var(--bg)',
                border: '1px solid var(--line)',
                color: 'var(--text)',
                padding: '6px 10px',
                borderRadius: '6px',
                fontSize: '12px',
              }}
            >
              <option value="ratio">Ratio (Value/Weight)</option>
              <option value="value">Priority Score</option>
              <option value="weight">Unit Weight</option>
              <option value="quantity">Available Quantity</option>
              <option value="name">Name</option>
            </select>
            <button
              className="btn btn-secondary"
              style={{ padding: '6px 10px', fontSize: '12px' }}
              onClick={() => setSortAsc(!sortAsc)}
            >
              {sortAsc ? '▲ Asc' : '▼ Desc'}
            </button>
          </div>
        </div>

        {/* Resources Data Table */}
        <div style={{ overflowX: 'auto' }}>
          <table className="data-table">
            <thead>
              <tr>
                <th style={{ width: '40px', textAlign: 'center' }}>Opt</th>
                <th>ID</th>
                <th>Supply Name</th>
                <th>Category</th>
                <th style={{ textAlign: 'right' }}>Stock Units</th>
                <th style={{ textAlign: 'right' }}>Unit Wt</th>
                <th>Priority Score</th>
                <th style={{ textAlign: 'center' }}>Ratio</th>
                <th>Depot / Shelter</th>
                <th style={{ textAlign: 'right' }}>Actions</th>
              </tr>
            </thead>
            <tbody>
              {filteredResources.length === 0 ? (
                <tr>
                  <td colSpan="10" style={{ textAlign: 'center', padding: '32px', color: 'var(--muted)' }}>
                    No supply resources found matching criteria.
                  </td>
                </tr>
              ) : (
                filteredResources.map((item) => (
                  <tr key={item.id}>
                    {/* Checkbox for optimizer inclusion */}
                    <td style={{ textAlign: 'center' }}>
                      <input
                        type="checkbox"
                        checked={selectedCandidateIds.has(item.id)}
                        onChange={() => toggleCandidate(item.id)}
                        style={{ cursor: 'pointer' }}
                        title="Include in Knapsack Optimization"
                      />
                    </td>
                    <td style={{ color: 'var(--muted)', fontSize: '12px' }}>#{item.id}</td>
                    <td>
                      <strong style={{ color: 'var(--text)' }}>{item.name}</strong>
                    </td>
                    <td>
                      <span className={`category-badge ${getCategoryClass(item.category)}`}>
                        {item.category}
                      </span>
                    </td>
                    <td style={{ textAlign: 'right' }}>
                      <span
                        style={{
                          fontWeight: 700,
                          color:
                            item.available_quantity === 0
                              ? 'var(--critical)'
                              : item.available_quantity < 20
                              ? 'var(--warning)'
                              : 'var(--text)',
                        }}
                      >
                        {item.available_quantity}
                      </span>{' '}
                      <small style={{ color: 'var(--muted)', fontSize: '11px' }}>units</small>
                    </td>
                    <td style={{ textAlign: 'right', fontVariantNumeric: 'tabular-nums' }}>
                      {item.unit_weight} kg
                    </td>
                    <td>
                      <div className="score-bar-wrap">
                        <span style={{ fontWeight: 600, minWidth: '24px', fontSize: '12px' }}>
                          {item.value_score}
                        </span>
                        <div className="score-bar-track">
                          <div
                            className="score-bar-fill"
                            style={{ width: `${Math.min(100, item.value_score)}%` }}
                          />
                        </div>
                      </div>
                    </td>
                    <td style={{ textAlign: 'center' }}>
                      <span className="ratio-pill">{item.ratio.toFixed(1)}</span>
                    </td>
                    <td style={{ fontSize: '12px', color: 'var(--muted)' }}>
                      {item.shelter_name || 'Central Depot'}
                    </td>
                    <td style={{ textAlign: 'right', whiteSpace: 'nowrap' }}>
                      <button
                        className="btn btn-secondary"
                        style={{ padding: '4px 8px', fontSize: '11.5px', marginRight: '6px' }}
                        onClick={() => openAllocateModal(item)}
                        disabled={item.available_quantity <= 0}
                        title="Dispatch / Allocate units"
                      >
                        🚚 Allocate
                      </button>
                      <button
                        className="btn btn-secondary"
                        style={{ padding: '4px 8px', fontSize: '11.5px', marginRight: '6px' }}
                        onClick={() => openEditModal(item)}
                        title="Edit details"
                      >
                        ✏️
                      </button>
                      <button
                        className="btn btn-secondary"
                        style={{ padding: '4px 8px', fontSize: '11.5px', color: 'var(--critical)' }}
                        onClick={() => handleDeleteResource(item)}
                        title="Delete resource"
                      >
                        🗑️
                      </button>
                    </td>
                  </tr>
                ))
              )}
            </tbody>
          </table>
        </div>
      </div>

      {/* =========================================================================
          SECTION 3: DISTRIBUTION HISTORY LOG
          ========================================================================= */}
      <div className="card">
        <div className="card-head">
          <div className="title">
            <span>📋 Emergency Distribution History</span>
          </div>
          <span className="tag">{allocations.length} Dispatches Logged</span>
        </div>

        <div style={{ overflowX: 'auto' }}>
          <table className="data-table">
            <thead>
              <tr>
                <th>Alloc ID</th>
                <th>Resource Name</th>
                <th>Category</th>
                <th>Destination</th>
                <th style={{ textAlign: 'right' }}>Dispatched Qty</th>
                <th style={{ textAlign: 'right' }}>Total Weight</th>
                <th>Timestamp</th>
              </tr>
            </thead>
            <tbody>
              {allocations.length === 0 ? (
                <tr>
                  <td colSpan="7" style={{ textAlign: 'center', padding: '24px', color: 'var(--muted)' }}>
                    No distributions recorded yet. Use the "Allocate" button to dispatch supplies.
                  </td>
                </tr>
              ) : (
                allocations.map((alloc) => (
                  <tr key={alloc.id}>
                    <td style={{ color: 'var(--muted)', fontSize: '12px' }}>#{alloc.id}</td>
                    <td>
                      <strong style={{ color: 'var(--text)' }}>{alloc.resource_name}</strong>
                    </td>
                    <td>
                      <span className={`category-badge ${getCategoryClass(alloc.category)}`}>
                        {alloc.category}
                      </span>
                    </td>
                    <td>
                      <span style={{ color: 'var(--primary)', fontSize: '12.5px' }}>
                        {alloc.shelter_name || (alloc.request_id ? `Request #${alloc.request_id}` : 'Field Dispatch')}
                      </span>
                    </td>
                    <td style={{ textAlign: 'right', fontWeight: 600 }}>
                      {alloc.allocated_quantity} units
                    </td>
                    <td style={{ textAlign: 'right', fontVariantNumeric: 'tabular-nums' }}>
                      {alloc.total_weight} kg
                    </td>
                    <td style={{ fontSize: '12px', color: 'var(--muted)' }}>{alloc.allocation_date}</td>
                  </tr>
                ))
              )}
            </tbody>
          </table>
        </div>
      </div>

      {/* =========================================================================
          MODAL 1: ADD RESOURCE MODAL
          ========================================================================= */}
      {showAddModal && (
        <div className="modal-backdrop" onClick={() => setShowAddModal(false)}>
          <div className="modal-window" onClick={(e) => e.stopPropagation()}>
            <div className="modal-header">
              <h3>+ Add Emergency Relief Supply</h3>
              <button className="modal-close" onClick={() => setShowAddModal(false)}>
                ×
              </button>
            </div>
            <form onSubmit={handleCreateResource}>
              <div className="modal-body">
                <div className="form-group">
                  <label>Supply Name *</label>
                  <input
                    type="text"
                    required
                    placeholder="e.g. High-Calorie Ration Packs"
                    value={itemForm.name}
                    onChange={(e) => setItemForm({ ...itemForm, name: e.target.value })}
                  />
                </div>

                <div className="form-row">
                  <div className="form-group">
                    <label>Category *</label>
                    <select
                      value={itemForm.category}
                      onChange={(e) => setItemForm({ ...itemForm, category: e.target.value })}
                    >
                      {CATEGORIES.filter((c) => c !== 'All').map((cat) => (
                        <option key={cat} value={cat}>
                          {cat}
                        </option>
                      ))}
                    </select>
                  </div>
                  <div className="form-group">
                    <label>Target Shelter / Depot</label>
                    <select
                      value={itemForm.shelter_id}
                      onChange={(e) => setItemForm({ ...itemForm, shelter_id: e.target.value })}
                    >
                      <option value="">Central Supply Depot</option>
                      {shelters.map((s) => (
                        <option key={s.id} value={s.id}>
                          {s.name}
                        </option>
                      ))}
                    </select>
                  </div>
                </div>

                <div className="form-row">
                  <div className="form-group">
                    <label>Available Units *</label>
                    <input
                      type="number"
                      min="0"
                      required
                      value={itemForm.available_quantity}
                      onChange={(e) => setItemForm({ ...itemForm, available_quantity: e.target.value })}
                    />
                  </div>
                  <div className="form-group">
                    <label>Unit Weight (kg) *</label>
                    <input
                      type="number"
                      step="0.1"
                      min="0.1"
                      required
                      value={itemForm.unit_weight}
                      onChange={(e) => setItemForm({ ...itemForm, unit_weight: e.target.value })}
                    />
                  </div>
                  <div className="form-group">
                    <label>Priority Value Score (1 - 100) *</label>
                    <input
                      type="number"
                      min="1"
                      max="100"
                      required
                      value={itemForm.value_score}
                      onChange={(e) => setItemForm({ ...itemForm, value_score: e.target.value })}
                    />
                  </div>
                </div>
              </div>

              <div className="modal-footer">
                <button type="button" className="btn btn-secondary" onClick={() => setShowAddModal(false)}>
                  Cancel
                </button>
                <button type="submit" className="btn btn-primary">
                  Save Resource
                </button>
              </div>
            </form>
          </div>
        </div>
      )}

      {/* =========================================================================
          MODAL 2: EDIT RESOURCE MODAL
          ========================================================================= */}
      {showEditModal && activeItem && (
        <div className="modal-backdrop" onClick={() => setShowEditModal(false)}>
          <div className="modal-window" onClick={(e) => e.stopPropagation()}>
            <div className="modal-header">
              <h3>✏️ Edit Resource #{activeItem.id}</h3>
              <button className="modal-close" onClick={() => setShowEditModal(false)}>
                ×
              </button>
            </div>
            <form onSubmit={handleUpdateResource}>
              <div className="modal-body">
                <div className="form-group">
                  <label>Supply Name *</label>
                  <input
                    type="text"
                    required
                    value={itemForm.name}
                    onChange={(e) => setItemForm({ ...itemForm, name: e.target.value })}
                  />
                </div>

                <div className="form-row">
                  <div className="form-group">
                    <label>Category *</label>
                    <select
                      value={itemForm.category}
                      onChange={(e) => setItemForm({ ...itemForm, category: e.target.value })}
                    >
                      {CATEGORIES.filter((c) => c !== 'All').map((cat) => (
                        <option key={cat} value={cat}>
                          {cat}
                        </option>
                      ))}
                    </select>
                  </div>
                  <div className="form-group">
                    <label>Assigned Shelter</label>
                    <select
                      value={itemForm.shelter_id}
                      onChange={(e) => setItemForm({ ...itemForm, shelter_id: e.target.value })}
                    >
                      <option value="">Central Depot</option>
                      {shelters.map((s) => (
                        <option key={s.id} value={s.id}>
                          {s.name}
                        </option>
                      ))}
                    </select>
                  </div>
                </div>

                <div className="form-row">
                  <div className="form-group">
                    <label>Available Units</label>
                    <input
                      type="number"
                      min="0"
                      required
                      value={itemForm.available_quantity}
                      onChange={(e) => setItemForm({ ...itemForm, available_quantity: e.target.value })}
                    />
                  </div>
                  <div className="form-group">
                    <label>Unit Weight (kg)</label>
                    <input
                      type="number"
                      step="0.1"
                      min="0.1"
                      required
                      value={itemForm.unit_weight}
                      onChange={(e) => setItemForm({ ...itemForm, unit_weight: e.target.value })}
                    />
                  </div>
                  <div className="form-group">
                    <label>Priority Value Score</label>
                    <input
                      type="number"
                      min="1"
                      max="100"
                      required
                      value={itemForm.value_score}
                      onChange={(e) => setItemForm({ ...itemForm, value_score: e.target.value })}
                    />
                  </div>
                </div>
              </div>

              <div className="modal-footer">
                <button type="button" className="btn btn-secondary" onClick={() => setShowEditModal(false)}>
                  Cancel
                </button>
                <button type="submit" className="btn btn-primary">
                  Update Resource
                </button>
              </div>
            </form>
          </div>
        </div>
      )}

      {/* =========================================================================
          MODAL 3: ALLOCATE / DISPATCH MODAL
          ========================================================================= */}
      {showAllocateModal && activeItem && (
        <div className="modal-backdrop" onClick={() => setShowAllocateModal(false)}>
          <div className="modal-window" onClick={(e) => e.stopPropagation()}>
            <div className="modal-header">
              <h3>🚚 Dispatch Emergency Supplies</h3>
              <button className="modal-close" onClick={() => setShowAllocateModal(false)}>
                ×
              </button>
            </div>
            <form onSubmit={handleAllocate}>
              <div className="modal-body">
                <div
                  style={{
                    background: 'var(--bg)',
                    padding: '12px',
                    borderRadius: '6px',
                    border: '1px solid var(--line)',
                    marginBottom: '16px',
                  }}
                >
                  <div style={{ fontWeight: 700, color: 'var(--text)', fontSize: '15px' }}>
                    {activeItem.name}
                  </div>
                  <div style={{ fontSize: '12px', color: 'var(--muted)', marginTop: '4px' }}>
                    Available in stock:{' '}
                    <strong style={{ color: 'var(--success)' }}>
                      {activeItem.available_quantity} units
                    </strong>{' '}
                    • Unit weight: {activeItem.unit_weight} kg
                  </div>
                </div>

                <div className="form-group">
                  <label>Quantity to Dispatch *</label>
                  <input
                    type="number"
                    min="1"
                    max={activeItem.available_quantity}
                    required
                    value={allocForm.quantity}
                    onChange={(e) => setAllocForm({ ...allocForm, quantity: e.target.value })}
                  />
                  <small style={{ color: 'var(--muted)', fontSize: '11px', marginTop: '4px', display: 'block' }}>
                    Total dispatch weight:{' '}
                    <strong>{(Number(allocForm.quantity || 0) * activeItem.unit_weight).toFixed(1)} kg</strong>
                  </small>
                </div>

                <div className="form-group">
                  <label>Destination Shelter *</label>
                  <select
                    value={allocForm.shelter_id}
                    onChange={(e) => setAllocForm({ ...allocForm, shelter_id: e.target.value })}
                  >
                    <option value="">-- Select Destination Shelter --</option>
                    {shelters.map((s) => (
                      <option key={s.id} value={s.id}>
                        {s.name} (Capacity: {s.occupied}/{s.capacity})
                      </option>
                    ))}
                  </select>
                </div>
              </div>

              <div className="modal-footer">
                <button type="button" className="btn btn-secondary" onClick={() => setShowAllocateModal(false)}>
                  Cancel
                </button>
                <button type="submit" className="btn btn-primary">
                  Confirm Dispatch
                </button>
              </div>
            </form>
          </div>
        </div>
      )}
    </div>
  );
}
