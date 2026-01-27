//! Integration tests for IMUFusion

use atomik_edge_sensor::IMUFusion;

#[test]
fn test_load() {
    let mut manager = IMUFusion::new();
    manager.load(0x1234567890ABCDEF);
    assert_eq!(manager.get_initial_state(), 0x1234567890ABCDEF);
    assert_eq!(manager.get_accumulator(), 0);
}

#[test]
fn test_accumulate() {
    let mut manager = IMUFusion::new();
    manager.load(0);
    manager.accumulate(0x1111111111111111);
    assert_eq!(manager.get_accumulator(), 0x1111111111111111);
    manager.accumulate(0x2222222222222222);
    assert_eq!(manager.get_accumulator(), 0x3333333333333333);
}

#[test]
fn test_reconstruct() {
    let mut manager = IMUFusion::new();
    manager.load(0xAAAAAAAAAAAAAAAA);
    manager.accumulate(0x5555555555555555);
    // 0xAAAA XOR 0x5555 = 0xFFFF
    assert_eq!(manager.reconstruct(), 0xFFFFFFFFFFFFFFFF);
}

#[test]
fn test_self_inverse() {
    let mut manager = IMUFusion::new();
    manager.load(0xAAAAAAAAAAAAAAAA);
    let delta = 0x1234567890ABCDEF;
    manager.accumulate(delta);
    manager.accumulate(delta);  // Apply same delta twice
    // Self-inverse: delta XOR delta = 0
    assert!(manager.is_accumulator_zero());
    assert_eq!(manager.reconstruct(), 0xAAAAAAAAAAAAAAAA);
}

#[test]
fn test_rollback() {
    let mut manager = IMUFusion::new();
    manager.load(0);
    manager.accumulate(0x1111111111111111);
    manager.accumulate(0x2222222222222222);
    manager.accumulate(0x4444444444444444);
    assert_eq!(manager.get_accumulator(), 0x7777777777777777);
    
    // Rollback last 2 operations
    let count = manager.rollback(2);
    assert_eq!(count, 2);
    assert_eq!(manager.get_accumulator(), 0x1111111111111111);
}
